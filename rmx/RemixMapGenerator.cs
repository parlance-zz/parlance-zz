using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Tilemaps;
using System;
using System.IO;

using System.Runtime.InteropServices;   // used for native plugin imports

#if UNITY_EDITOR    // editor integration for compiled prefab export
using UnityEditor;
#endif

public class RemixMapGenerator : MonoBehaviour
{
    public string exportPath = "";  // prefab asset path we will compile to

    public Grid[] mergeSelectedGrids = null;

    [SerializeField, HideInInspector] 
    private List<CompositeTile> tiles = new List<CompositeTile>();     // list of all composite (multi-layer) tiles from all compiled sample maps, index used as integer (ushort) tile id
    [SerializeField, HideInInspector]
    private List<CompositeMap> sampleMaps = new List<CompositeMap>(); // collection of all compiled sample maps
    [SerializeField, HideInInspector]
    private List<CompositePrefab> compositePrefabs = new List<CompositePrefab>(); // collection of compiled prefabs in sample maps

    private UInt64 generator = 0;   // handle to native plugin Generator

    [SerializeField]
    public LayerMask staticBlockerLayers = 0;
    [SerializeField]
    public LayerMask dynamicBlockerLayers = 0;

    [SerializeField]
    private int numTotalContextBlocks = 0, numUniqueTiles = 0;  // just for book keeping and easy verification in inspector

    // native plugin imports
    [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
    private static extern UInt64 RMX_CreateGenerator(int numCompositeTiles, int numSampleMaps);
    [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
    private static extern void RMX_Generator_Destroy(UInt64 generator);
    [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
    private static extern unsafe void RMX_Generator_SetTileAdjacency(UInt64 generator, int tileId, int direction, int numAdjacents, ushort* adjacentIds, uint *adjacentWeights);
    [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
    private static extern void RMX_Generator_SetTileFlags(UInt64 generator, int tileId, uint tileFlags);
    [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
    private static extern unsafe void RMX_Generator_SetTileLocations(UInt64 generator, int tileId, int numLocations, Vector3Int *locations, int autoSpawnLimit);
    [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
    private static extern unsafe void RMX_Generator_SetSampleMap(UInt64 generator, int mapIndex, int sizeX, int sizeY, ushort *mapData);

    // represents a prefab of grouped composite tiles for manual painting
    [Serializable]
    public class CompositePrefab
    {
        public string name;
        public int index;

        public List<Vector3Int> tileLocations = new List<Vector3Int>();
        public Vector2Int handleLocation;
        
        public CompositePrefab(int newIndex, string name)
        {
            index = newIndex;
            this.name = name;
        }
    }

    // CompositeTile components
    [Serializable]
    public struct AdjacencyList
    {
        public List<ushort> adjacents;
        public uint[] adjacentWeights;
        public Dictionary<ushort, uint> tempAdjacentWeights;
    }

    // composite (multi-layer) tile. can be a block of 3x3 tiles or a single tile. used as integer id in generator. contains list of TileBase references for painting back into output layers
    [Serializable]
    public class CompositeTile
    {
        public const ushort NULL_TILE_ID  = 0xFFFF; // used to represent any empty tile

        public const uint FLAG_STATIC_BLOCKER = 1;
        public const uint FLAG_DYNAMIC_BLOCKER = 2;
        public const uint FLAG_OOB_ADJACENT = 4;

        public static readonly Vector2Int[] adjacencyOffsets = { new Vector2Int { x = -1, y = -1 },
                                                                 new Vector2Int { x =  0, y = -1 },
                                                                 new Vector2Int { x =  1, y = -1 },
                                                                 new Vector2Int { x = -1, y =  0 },
                                                                 new Vector2Int { x =  1, y =  0 },
                                                                 new Vector2Int { x = -1, y =  1 },
                                                                 new Vector2Int { x =  0, y =  1 },
                                                                 new Vector2Int { x =  1, y =  1 } };

        public TileBase[] layers;        // the actual tile asset references
        
        public AdjacencyList[] adjacents = new AdjacencyList[8];            // used to define adjacency constraints for this tile
        public List<Vector3Int> contextBlocks = new List<Vector3Int>();     // collection of context blocks for which this tile appears at the center, used for contextual tile selection. x and y is coord in sample map, z is index of sample map

        public ushort id;
        public int autoSpawnLimit = -1;

        public uint flags = 0;

        // constructs a new composite tile with specified integer id and from the specified layers / position.
        public CompositeTile(ushort newId, Tilemap[] layers, int x, int y, LayerMask staticBlockingLayers, LayerMask dynamicBlockingLayers)
        {
            id = newId; if (id == 0) autoSpawnLimit = 0;

            this.layers = new TileBase[layers.Length];
            for (int l = 0; l < layers.Length; l++)
            {
                this.layers[l] = layers[l].GetTile(new Vector3Int(x, y, 0));
                if (this.layers[l] != null)
                {
                    if ((layers[l].gameObject.layer & staticBlockingLayers) != 0) flags |= FLAG_STATIC_BLOCKER;
                    if ((layers[l].gameObject.layer & dynamicBlockingLayers) != 0) flags |= FLAG_DYNAMIC_BLOCKER;
                }
            }

            for (int a = 0; a < adjacencyOffsets.Length; a++)
            {
                bool oobAdjacent = true;

                for (int l = 0; l < layers.Length; l++)
                {
                    if (layers[l].GetTile(new Vector3Int(x + adjacencyOffsets[a].x, y + adjacencyOffsets[a].y, 0)) != null)
                    {
                        oobAdjacent = false;
                        break;
                    }
                }

                if (oobAdjacent)
                {
                    flags |= FLAG_OOB_ADJACENT;
                    break;
                }
            }
        }

        public bool Equals(CompositeTile other) // check if composite tile is identical to another (including all layers)
        {
            if (other.flags != flags) return false;
            int maxLayers = layers.Length; if (other.layers.Length > maxLayers) maxLayers = other.layers.Length;

            for (int l = 0; l < maxLayers; l++)
            {
                TileBase tile1 = null; if (l < layers.Length) tile1 = layers[l];
                TileBase tile2 = null; if (l < other.layers.Length) tile2 = other.layers[l];
                if (tile1 != tile2) return false;
         
            }
            return true;
        }
        
        public void Paint(Tilemap[] layers, int x, int y)  // paints this composite tile to the specified tilemap layers and position
        {
            for (int l = 0; l < layers.Length; l++)
            {    
                TileBase tile = null; if (l <= (this.layers.Length - 1)) tile = this.layers[l];
                layers[l].SetTile(new Vector3Int(x, y, 0), tile);
            }
        }
    }

    // represents a simple integer id based map of interlocking multi-layer CompositeTile blocks
    [Serializable]
    public class CompositeMap
    {
        public ushort[] map;   // actual map data
        public int sizeX;      // map dimensions
        public int sizeY;

        public string name = "";       // name of the grid gameobject from which this map was compiled, or empty otherwise

        public BoundsInt sampleBounds;

        public CompositeMap(int width, int height)
        {
            sizeX = width;
            sizeY = height;

            map = new ushort[sizeX * sizeY];
            for (int i = 0; i < sizeX * sizeY; i++) map[i] = CompositeTile.NULL_TILE_ID;
        }

        public CompositeMap(Tilemap[] sampleLayers, string sampleName, RemixMapGenerator mapGen)
        {
            name = sampleName;

            // find extent bounds of sample
            BoundsInt mapBounds = sampleLayers[0].cellBounds;
            foreach (Tilemap layer in sampleLayers) // grow bounds to the largest size that covers all layers
            {
                if ((layer.cellBounds.size.x > 0) && (layer.cellBounds.size.y > 0)) // if tilemap is empty ignore bounds
                {
                    if (layer.cellBounds.xMin < mapBounds.xMin) mapBounds.xMin = layer.cellBounds.xMin;
                    if (layer.cellBounds.xMax > mapBounds.xMax) mapBounds.xMax = layer.cellBounds.xMax;
                    if (layer.cellBounds.yMin < mapBounds.yMin) mapBounds.yMin = layer.cellBounds.yMin;
                    if (layer.cellBounds.yMax > mapBounds.yMax) mapBounds.yMax = layer.cellBounds.yMax;
                }
            }

            sizeX = mapBounds.size.x + 2;   // includes room for border OOB tiles
            sizeY = mapBounds.size.y + 2;
            sampleBounds = mapBounds;
            map = new ushort[sizeX * sizeY];

            // fill map data, requesting new tile ids from the generator if we see a previously unseen composite tile
            for (int y = 0; y < sizeY; y++)
            {
                for (int x = 0; x < sizeX; x++)
                {
                    map[y * sizeX + x] = mapGen.FindCompositeTileId(sampleLayers, x + mapBounds.xMin - 1, y + mapBounds.yMin - 1);
                }
            }
        }

        public ushort GetTileId(int x, int y)   // gets the tile id at the specified position. if position is invalid returns null.
        {
            if ((x < 0) || (y < 0) || (x >= sizeX) || (y >= sizeY)) return CompositeTile.NULL_TILE_ID;
            return map[y * sizeX + x];
        }

        public void SetTileId(int x, int y, ushort tileId) // set the tile id at the specified position, if position is invalid does nothing.
        {
            if ((x < 0) || (y < 0) || (x >= sizeX) || (y >= sizeY)) return;
            map[y * sizeX + x] = tileId;
        }
    }

    // represents a generating map in progress
    public class SuperMap : IDisposable
    {
        // native plugin imports
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern UInt64 RMX_Generator_CreateSuperMap(UInt64 generator, int sizeX, int sizeY, UInt64 seed);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RMX_SuperMap_Destroy(UInt64 superMap);

        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe void RMX_SuperMap_GetDensityMap(UInt64 superMap, float *map);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe void RMX_SuperMap_GetSuperTiles(UInt64 superMap, UInt64 *superTiles);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe void RMX_SuperMap_GetTargetMap(UInt64 superMap, ushort *mapData);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe void RMX_SuperMap_GetContextMap(UInt64 superMap, Vector3Int *contextData);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern Vector2Int RMX_SuperMap_GetLastFailPos(UInt64 superMap);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe void RMX_SuperMap_SetSuperTiles(UInt64 superMap, UInt64 *superTiles);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe void RMX_SuperMap_SetTargetMap(UInt64 superMap, ushort *mapData);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe void RMX_SuperMap_SetContextMap(UInt64 superMap, Vector3Int *contextData);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern void RMX_SuperMap_SetParams(UInt64 superMap, int constraintPropMaxDensity, int flags);

        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern Vector2Int RMX_SuperMap_FindNextGenLocation(UInt64 superMap, int direction, uint flags, int referenceX, int referenceY, float localityBias);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe int RMX_SuperMap_PaintTiles(UInt64 superMap, int numTiles, Vector2Int *locations, ushort *tileIds, int spaceBias, int temperature, uint flags);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe int RMX_SuperMap_Constrain(UInt64 superMap, int numTiles, Vector2Int* locations, ushort* tileIds, uint flags);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern unsafe int RMX_SuperMap_ClearTiles(UInt64 superMap, int numTiles, Vector2Int *locations, uint flags);
        [DllImport("rmxLib", CallingConvention = CallingConvention.Cdecl)]
        private static extern UInt64 RMX_SuperMap_GetRandom(UInt64 superMap);

        private const int CONSTRAIN_FLAGS_BANISH_ON_FAIL = 1;
        private const int SUPERMAP_FLAGS_USE_DISTRIBUTION_BIAS = 1;
        private const int FIND_FLAGS_FIND_OPEN = 1;
        private const int CLEARTILES_FLAGS_CLEAR_ADJACENT_CONTEXTS = 1;
        private const int CLEARTILES_FLAGS_CLEAR_OOB_TILES = 2;

        private const int DEFAULT_CONTEXT_BLOCK_WIDTH = 25;
        private const int DEFAULT_CONTEXT_BLOCK_HEIGHT = 25;
        private const int MIN_CONSTRAINT_PROP_DENSITY = 10;

        public int sizeX; // map dimensions
        public int sizeY;

        public Vector2Int lastFailPos = new Vector2Int(-1, -1);
        public Vector2Int lastGenPos = new Vector2Int(-1, -1);

        public CompositeMap targetMap;  // the map of actual placed composite tile ids in the map we are generating

        public Grid target;             // grid containing the tilemap layers we are generating into
        private Tilemap[] outputLayers; // tilemap components in target grid

        private RemixMapGenerator generator; // reference to the generator used to create this SuperMap
        private UInt64 superMap = 0;         // handle to native plugin SuperMap

        private List<Snapshot> snapshots = new List<Snapshot>(); // collection of historical snapshots
        private int paintSuccessCount = 0;                       // number of tiles generated successfully since the last snapshot

        private int maxSnapshotsToKeep = 10;
        private int maxTilesBeforeSnapshot = 2500;
        private int constraintPropMaxDensity;
        private int supermapFlags = 0;

        private float[] densityMap;      // used for debug drawing density
        private Vector3Int[] contextMap; // used for debug context drawing

        private class Snapshot  // represents a snapshot of the generating map, including all supertiles and composite tiles
        {
            public UInt64[] backupSuperTiles;
            public ushort[] backupTargetMap;
            public Vector3Int[] backupContextMap;
            public bool removable;          // if this is set to false then this snapshot will not be removed, even after recovery
        }

        // creates a new generating map using the specified dimensions and referenced tileIds, into the target grid
        public SuperMap(RemixMapGenerator rmxGenerator, int width, int height, Grid targetGrid, ulong randomSeed)
        {
            sizeX = width;
            sizeY = height;

            target = targetGrid;
            outputLayers = target.GetComponentsInChildren<Tilemap>();

            if (randomSeed == 0) randomSeed = (ulong)DateTime.Now.Ticks;

            generator = rmxGenerator; generator.InitGenerator();
            superMap = RMX_Generator_CreateSuperMap(rmxGenerator.generator, width, height, randomSeed);

            constraintPropMaxDensity = rmxGenerator.tiles.Count / 3; if (constraintPropMaxDensity < MIN_CONSTRAINT_PROP_DENSITY) constraintPropMaxDensity = MIN_CONSTRAINT_PROP_DENSITY;
            RMX_SuperMap_SetParams(superMap, constraintPropMaxDensity, supermapFlags);

            densityMap = new float[sizeX * sizeY];
            contextMap = new Vector3Int[sizeX * sizeY];
            targetMap = new CompositeMap(width, height);
        }
  
        // set various parameters for generation
        public void SetParams(int maxSnapshotsToKeep, int maxTilesBeforeSnapshot, int maxPropDensity, bool useDistributionBias = false)
        {
            if (maxSnapshotsToKeep >= 1) { this.maxSnapshotsToKeep = maxSnapshotsToKeep; }
            if (maxTilesBeforeSnapshot >= 1) { this.maxTilesBeforeSnapshot = maxTilesBeforeSnapshot; }
            if ((maxPropDensity >= MIN_CONSTRAINT_PROP_DENSITY) && (maxPropDensity <= CompositeTile.NULL_TILE_ID))
            {
                constraintPropMaxDensity = maxPropDensity;

                supermapFlags = 0; if (useDistributionBias) supermapFlags |= SUPERMAP_FLAGS_USE_DISTRIBUTION_BIAS;
                RMX_SuperMap_SetParams(superMap, constraintPropMaxDensity, supermapFlags);
            }
        }

        public int GetRandom(int range)   // generates a random number from the supermap seeded rng, from 0 (inclusive) to range (EXclusive)
        {
            return (int)(RMX_SuperMap_GetRandom(superMap) % (UInt64)range);
        }

        // take a snapshot of the generating map for backup, snapshots can be set to unremovable (manual snapshot after setting up prefabs and borders)
        public void TakeSnapshot(bool removable = true)
        {
            Snapshot newSnapshot = new Snapshot();
            newSnapshot.removable = removable;
            newSnapshot.backupTargetMap = new ushort[sizeX * sizeY];
            newSnapshot.backupContextMap = new Vector3Int[sizeX * sizeY];

            int len = (generator.tiles.Count + 63) >> 6;
            newSnapshot.backupSuperTiles = new UInt64[len * sizeX * sizeY];
            
            unsafe
            {
                fixed (ushort* mapData = &newSnapshot.backupTargetMap[0]) { RMX_SuperMap_GetTargetMap(superMap, mapData); }
                fixed (UInt64* superTileData = &newSnapshot.backupSuperTiles[0]) { RMX_SuperMap_GetSuperTiles(superMap, superTileData); }
                fixed (Vector3Int* contextData = &newSnapshot.backupContextMap[0]) { RMX_SuperMap_GetContextMap(superMap, contextData); }
            }

            snapshots.Add(newSnapshot);

            int numRemovableSnapshots = 0; for (int i = 0; i < snapshots.Count; i++) if (snapshots[i].removable) numRemovableSnapshots++;
            if (numRemovableSnapshots > maxSnapshotsToKeep)
            {
                for (int i = 0; i < snapshots.Count; i++)
                {
                    if (snapshots[i].removable)
                    {
                        snapshots.RemoveAt(i);
                        return;
                    }
                }
            }
        }

        // recovers the generating map state to the latest snapshot, returns false if no snapshots to recover. if clearNextContext is set the contexts adjacent to the next gen pos are erased (to avoid regening the same thing)
        public bool RecoverLastSnapshot(bool paintTarget = false)
        {
            if (snapshots.Count == 0) return false;
            Snapshot lastSnapshot = snapshots[snapshots.Count - 1];

            unsafe
            {
                fixed (ushort* mapData = &lastSnapshot.backupTargetMap[0]) { RMX_SuperMap_SetTargetMap(superMap, mapData); }
                fixed (UInt64* superTileData = &lastSnapshot.backupSuperTiles[0]) { RMX_SuperMap_SetSuperTiles(superMap, superTileData); }
                fixed (Vector3Int* contextData = &lastSnapshot.backupContextMap[0]) { RMX_SuperMap_SetContextMap(superMap, contextData); }

                fixed (ushort* mapData = &targetMap.map[0]) { RMX_SuperMap_GetTargetMap(superMap, mapData); }
            }

            if (paintTarget) PaintTarget();
            if (lastSnapshot.removable) snapshots.Remove(lastSnapshot);
            return true;
        }

        // find the best position to continue generation
        public Vector2Int FindNextGenPos(uint flags = 0, float localityBias = 0.0f)
        {
            int referenceX = targetMap.sizeX / 2, referenceY = targetMap.sizeY / 2;

            /*
            float fx = 0.0f, fy = 0.0f, fWeight = 0.0f;
            unsafe
            {
                fixed (float* mapData = &densityMap[0])
                {
                    RMX_SuperMap_GetDensityMap(superMap, mapData);
                }
            }

            for (int y = 0; y < sizeY; y++)
            {
                for (int x = 0; x < sizeX; x++)
                {
                    float weight = 1.0f - densityMap[y * sizeX + x];
                    fx += ((float)(x)) * weight;
                    fy += ((float)(y)) * weight;
                    fWeight += weight;
                }
            }

            if (fWeight > 0.0f)
            {
                referenceX = (int)(fx / fWeight);
                referenceY = (int)(fy / fWeight);

                if (referenceX < 0) referenceX = 0;
                if (referenceY < 0) referenceY = 0;
                if (referenceX >= sizeX) referenceX = sizeX - 1;
                if (referenceY >= sizeY) referenceY = sizeY - 1;

            }
            */

            
            /*
            if ((lastGenPos.x != -1) && (lastGenPos.y != -1))
            {
                referenceX = lastGenPos.x;
                referenceY = lastGenPos.y;
            }
            */

            /*
            if ((lastGenPos.x != -1) && (lastGenPos.y != -1))
            {
                referenceX = (referenceX + lastGenPos.x) / 2;
                referenceY = (referenceY + lastGenPos.y) / 2;
            }
            */

            int direction = GetRandom(4);
            return RMX_SuperMap_FindNextGenLocation(superMap, direction, flags, referenceX, referenceY, localityBias);
        }

        // generates part of the output map. returns 1 when finished, otherwise 0. this can be run concurrently on as many separate supermaps at once as desired in low priority background threads (you could simultaneously do like 3-5 rooms at once to save time)
        public bool GenerateMap(int numIterations, int temperature, bool paintTarget = false, float localityBias = 0.0f, bool findOpen = false)
        {
            uint findFlags = 0; if (findOpen) findFlags |= FIND_FLAGS_FIND_OPEN;

            for (int i = 0; i < numIterations; i++)
            {
                Vector2Int genPos = FindNextGenPos(findFlags, localityBias); if (genPos == new Vector2Int(-1, -1)) return true;
                PaintTile(genPos.x, genPos.y, CompositeTile.NULL_TILE_ID, temperature, paintTarget);
            }

            return false;
        }

        public bool PaintOOBRect(int x, int y, int width, int height)
        {
            if (x < 0) { width += x; x = 0; }
            if (y < 0) { height += y; y = 0; }
            if ((x + width) > sizeX) width = sizeX - x;
            if ((y + height) > sizeY) height = sizeY - y;

            int numRectTiles = width * height;
            if (numRectTiles == 0) return true;

            Vector2Int[] rect = new Vector2Int[numRectTiles];

            for (int ty = 0; ty < height; ty++)
            {
                for (int tx = 0; tx < width; tx++)
                {
                    rect[ty * width + tx] = new Vector2Int(tx + x, ty + y);
                }
            }

            ushort[] rectTileIds = new ushort[numRectTiles];
            for (int i = 0; i < numRectTiles; i++) rectTileIds[i] = 0;

            unsafe
            {
                fixed (Vector2Int* locations = &rect[0])
                {
                    fixed (ushort* tileIds = &rectTileIds[0])
                    {
                        if (RMX_SuperMap_PaintTiles(superMap, numRectTiles, locations, tileIds, 0, 0, 0) != 1)
                        {
                            lastFailPos = RMX_SuperMap_GetLastFailPos(superMap);
                            return false;
                        }
                    }
                }

            }

            for (int i = 0; i < numRectTiles; i++) targetMap.SetTileId(rect[i].x, rect[i].y, 0);
            return true;
        }

        // attempts to place a tile into the generating map at the specified position with the specified random temperature. the tile id can be specified or omitted and generated contextually
        // returns the placed tile id on success, or null tile id on failure. PaintTile can be retried repeatedly at the same position with different tile ids however if no tiles are left to try at that position then the generating map will be reverted to a recent snapshot
        public ushort PaintTile(int x, int y, ushort tileId=CompositeTile.NULL_TILE_ID, int randomTemperature = 0, bool paintTarget = false)
        {
            if ((x < 0) || (y < 0) || (x >= sizeX) || (y >= sizeY) || ((tileId >= generator.tiles.Count) && (tileId < CompositeTile.NULL_TILE_ID))) return CompositeTile.NULL_TILE_ID;
            if (randomTemperature < 0) randomTemperature = 0;

            Vector2Int location = new Vector2Int(x, y);
            uint flags = 0; if (tileId == CompositeTile.NULL_TILE_ID) flags = CONSTRAIN_FLAGS_BANISH_ON_FAIL;
            int success; unsafe { success = RMX_SuperMap_PaintTiles(superMap, 1, &location, &tileId, 0, randomTemperature, flags); }
            if (success == 0)
            {
                lastFailPos = RMX_SuperMap_GetLastFailPos(superMap);

                int snapshotCount = snapshots.Count;
                RecoverLastSnapshot(paintTarget);
                //Debug.Log("Recovering snapshot - " + snapshots.Count.ToString());
                //Debug.Assert((paintSuccessCount > 0) || snapshots.Count > , "RMX Error : Unable to generate tiles. Check sample layout constraints.");
                return CompositeTile.NULL_TILE_ID; // fail
            }
            else if (success == 1)
            {
                if (tileId != CompositeTile.NULL_TILE_ID)
                {
                    targetMap.SetTileId(x, y, tileId); if (paintTarget) generator.tiles[tileId].Paint(outputLayers, x, y);  // if prop is successful write the tile to the compositetile map and paint to target output layers if needed
                    paintSuccessCount++;
                    if (paintSuccessCount >= maxTilesBeforeSnapshot)   // take a snapshot if we've successfully placed the required number of tiles
                    {
                        TakeSnapshot();
                        paintSuccessCount = 0;
                    }
                    lastGenPos = new Vector2Int(x, y);
                }
                return tileId; 
            }
            else
            {
                lastFailPos = RMX_SuperMap_GetLastFailPos(superMap);
                return CompositeTile.NULL_TILE_ID;
            }
        }

        // paint the compositetile map to the target output layers
        public void PaintTarget()
        {
            unsafe
            {
                fixed (ushort *mapData = &targetMap.map[0])
                {
                    RMX_SuperMap_GetTargetMap(superMap, mapData);
                }
            }
            for (int y = 0; y < sizeY; y++)
            {
                for (int x = 0; x < sizeX; x++)
                {
                    ushort tileId = targetMap.GetTileId(x, y);
                    if (tileId != CompositeTile.NULL_TILE_ID) generator.tiles[tileId].Paint(outputLayers, x, y);
                    else for (int l = 0; l < outputLayers.Length; l++) outputLayers[l].SetTile(new Vector3Int(x, y, 0), null);
                }
            }
        }

        // paints an invisible 1 tile border of OOB tiles around the edge of the generating map. this is usually the first thing you'll want to do with a new map, as this will enforce
        // border constraints as taken from the samples
        public bool PaintBorder()
        {
            int numBorderTiles = sizeX * 2 + (sizeY - 2) * 2;
            Vector2Int[] borderLocations = new Vector2Int[numBorderTiles];
            ushort[] borderTileIds = new ushort[numBorderTiles];

            borderLocations[0] = new Vector2Int(0, 0);
            for (int i = 0; i < numBorderTiles; i++)
            {
                borderTileIds[i] = 0;
                if (i > 0)
                {
                    borderLocations[i] = borderLocations[i - 1];
                    if (borderLocations[i].y == 0)
                    {
                        if (borderLocations[i].x < (sizeX - 1)) borderLocations[i].x++; else borderLocations[i].y++;
                    }
                    else if (borderLocations[i].x == (sizeX - 1))
                    {
                        if (borderLocations[i].y < (sizeY - 1)) borderLocations[i].y++; else borderLocations[i].x--;
                    }
                    else if (borderLocations[i].y == (sizeY - 1))
                    {
                        if (borderLocations[i].x > 0) borderLocations[i].x--; else borderLocations[i].y--;
                    }
                    else borderLocations[i].y--;
                }
            }

            unsafe
            {
                fixed (Vector2Int *locations = &borderLocations[0])
                {
                    fixed (ushort* tileIds = &borderTileIds[0])
                    {
                        if (RMX_SuperMap_PaintTiles(superMap, numBorderTiles, locations, tileIds, 0, 0, 0) != 1)
                        {
                            lastFailPos = RMX_SuperMap_GetLastFailPos(superMap);
                            return false;
                        }
                    }
                }
            }
            
            for (int i = 0; i < numBorderTiles; i++) targetMap.SetTileId(borderLocations[i].x, borderLocations[i].y, 0);
            return true;
        }

        // draw a prefab into the generating map. aborts and returns false if any tile can't be inserted
        public bool PaintPrefab(int x, int y, CompositePrefab prefab, bool paintTarget = false, bool paintRandomTile = false)
        {
            int tileLocationCount = prefab.tileLocations.Count;

            if (paintRandomTile)
            {
                ushort randomTileId = (ushort)prefab.tileLocations[GetRandom(tileLocationCount)].z;
                if (PaintTile(x, y, randomTileId, 0, paintTarget) != randomTileId)
                {
                    lastFailPos = RMX_SuperMap_GetLastFailPos(superMap);
                    return false;
                }
            }
            else
            {
                for (int t = 0; t < tileLocationCount; t++)
                {
                    ushort tileId = (ushort)prefab.tileLocations[t].z;
                    if (PaintTile(x + prefab.tileLocations[t].x - prefab.handleLocation.x, y + prefab.tileLocations[t].y - prefab.handleLocation.y, tileId, 0, paintTarget) != tileId)
                    {
                        lastFailPos = RMX_SuperMap_GetLastFailPos(superMap);
                        return false;
                    }
                }
            }
   
            return true;
        }

        // resets the super tiles at the specified positions and propagates the constraint reset
        public void ClearTiles(Vector2Int[] tileLocations, bool paintTarget = false, bool clearNeighboringContexts = false, bool clearOOBTiles = false)
        {
            if (tileLocations.Length == 0) return;
            uint flags = 0; if (clearNeighboringContexts) flags |= CLEARTILES_FLAGS_CLEAR_ADJACENT_CONTEXTS; if (clearOOBTiles) flags |= CLEARTILES_FLAGS_CLEAR_OOB_TILES;

            unsafe
            {
                fixed (Vector2Int *locations = &tileLocations[0])
                {
                    RMX_SuperMap_ClearTiles(superMap, tileLocations.Length, locations, flags);        
                }
            }

            for (int i = 0; i < tileLocations.Length; i++)
            {
                ushort existingTileId = targetMap.GetTileId(tileLocations[i].x, tileLocations[i].y);
                if ((existingTileId != 0) || clearOOBTiles)
                {
                    targetMap.SetTileId(tileLocations[i].x, tileLocations[i].y, CompositeTile.NULL_TILE_ID);
                    if (paintTarget) for (int l = 0; l < outputLayers.Length; l++) outputLayers[l].SetTile(new Vector3Int(tileLocations[i].x, tileLocations[i].y, 0), null);
                }
            }
        }

        // draw the "density" of possible tiles at each location in the generating map (only in editor scene view)
        public void DrawDebugDensity()
        {
            unsafe
            {
                fixed (float* mapData = &densityMap[0])
                {
                    RMX_SuperMap_GetDensityMap(superMap, mapData);
                }
            }
            for (int y = 0; y < sizeY; y++)
            {
                for (int x = 0; x < sizeX; x++)
                {
                    int tileOffset = y * sizeX + x;
                    if (targetMap.map[tileOffset] == CompositeTile.NULL_TILE_ID)
                    {
                        float density = densityMap[tileOffset]; Color color = new Color(1.0f - density, 1.0f, 0.0f, 0.75f);
                        RemixMapGenerator.DrawDebugCellHighlight(target, new Vector3Int(x, y, 0), color);
                    }
                }
            }
        }

        // draws a map of colored regions in scene view to illustrate regions taken directly from sample maps
        public void DrawDebugContexts()
        {
            int lastHashCode = 0;
            Color lastColor = new Color(0.0f, 0.0f, 0.0f, 1.0f);
            System.Random rng = new System.Random();

            unsafe
            {
                fixed (Vector3Int* contextData = &contextMap[0])
                {
                    RMX_SuperMap_GetContextMap(superMap, contextData);
                }
            }
            for (int y = 0; y < sizeY; y++)
            {
                for (int x = 0; x < sizeX; x++)
                {
                    Vector3Int context = contextMap[y * sizeX + x];

                    if (context.z != -1)
                    {
                        context.x -= x; context.y -= y;
                        int hashCode = context.GetHashCode() ^ ((context.GetHashCode()) << 16);
                        
                        if (hashCode != lastHashCode)
                        {
                            lastHashCode = hashCode;
                            rng = new System.Random(lastHashCode);
                            lastColor = new Color((float)rng.NextDouble(), (float)rng.NextDouble(), (float)rng.NextDouble(), 0.75f);
                        }

                        RemixMapGenerator.DrawDebugCellHighlight(target, new Vector3Int(x, y, 0), lastColor);
                    }
                }
            }
        }

        void IDisposable.Dispose()
        {
            if (superMap != 0) RMX_SuperMap_Destroy(superMap);
            superMap = 0;
        }
    }

    // free native memory when game object is destroyed
    private void OnDestroy()
    {
        if (generator != 0)
        {
            RMX_Generator_Destroy(generator);
            generator = 0;
        }
    }

    // find (or create) a new composite tile id from specified layers and position
    public ushort FindCompositeTileId(Tilemap[] layers, int x, int y)
    {
        ushort newId = (ushort)tiles.Count;
        CompositeTile composite = new CompositeTile(newId, layers, x, y, staticBlockerLayers, dynamicBlockerLayers);
        CompositeTile existing = tiles.Find(other => other.Equals(composite));
        if (existing == null)   // if tile already exists return the existing id, otherwise create new
        {
            tiles.Add(composite); numUniqueTiles++;
            return newId;
        }
        return existing.id;
    }

    public CompositePrefab FindPrefabByName(string name)
    {
        int compositePrefabCount = compositePrefabs.Count;
        for (int i = 0; i < compositePrefabCount; i++)
        {
            if (compositePrefabs[i].name == name) return compositePrefabs[i];
        }

        return null;
    }

    public bool MergeSample(Grid sampleGrid)
    {
        Tilemap[] layers = gameObject.GetComponentsInChildren<Tilemap>();
        Tilemap[] sampleLayers = sampleGrid.GetComponentsInChildren<Tilemap>();

        Grid grid = gameObject.GetComponent<Grid>();
        if (grid == null) return false;

        int numLayers = layers.Length; if (numLayers > sampleLayers.Length) numLayers = sampleLayers.Length;

        for (int l = 0; l < numLayers; l++)
        {
            sampleLayers[l].CompressBounds();

            for (int y = sampleLayers[l].cellBounds.yMin; y <= sampleLayers[l].cellBounds.yMax; y++)
            {
                for (int x = sampleLayers[l].cellBounds.xMin; x <= sampleLayers[l].cellBounds.xMax; x++)
                {
                    Vector3 sampleCellCenterWorld = (sampleGrid.CellToWorld(new Vector3Int(x, y, 0)) + sampleGrid.CellToWorld(new Vector3Int(x + 1, y + 1, 0))) * 0.5f;
                    Vector3Int translatedCellPos = grid.WorldToCell(sampleCellCenterWorld);
                    layers[l].SetTile(translatedCellPos, sampleLayers[l].GetTile(new Vector3Int(x, y, 0)));
                }
            }
        }

        return true;
    }

    // compile and use sample map (the generator can compile multiple maps)
    public void CompileSample(Grid sampleGrid)
    {
        Tilemap[] childTilemaps = sampleGrid.GetComponentsInChildren<Tilemap>();

        List<Tilemap> sampleLayers = new List<Tilemap>();
        foreach (Tilemap layer in childTilemaps)
        {
            layer.CompressBounds(); // shrink each layer bounds to extents

            RemixPrefab prefab = layer.gameObject.GetComponent<RemixPrefab>();
            if (prefab == null) sampleLayers.Add(layer);
            
        }

        CompositeMap sampleMap = new CompositeMap(sampleLayers.ToArray(), sampleGrid.name, this);
        Debug.Log("Compiling sample map '" + sampleGrid.name + "' : width : " + sampleMap.sizeX.ToString() + " height : " + sampleMap.sizeY.ToString());

        // create context blocks
        int mapIndex = sampleMaps.Count;
        for (int y = 1; y < sampleMap.sizeY - 1; y++)
        {
            for (int x = 1; x < sampleMap.sizeX - 1; x++)
            {
                ushort centerTile = sampleMap.GetTileId(x, y);
                if ((centerTile < tiles.Count) && (centerTile > 0))
                {
                    tiles[centerTile].contextBlocks.Add(new Vector3Int(x, y, mapIndex)); // keep list of context blocks per each tile, except OOB tile
                    numTotalContextBlocks++;
                }
            }
        }
  
        sampleMaps.Add(sampleMap);

        // create prefabs
        foreach (Tilemap layer in childTilemaps)
        {
            RemixPrefab prefab = layer.gameObject.GetComponent<RemixPrefab>();
            if (prefab != null)
            {
                CompositePrefab compPrefab = new CompositePrefab(compositePrefabs.Count, layer.name);
                for (int y = 1; y < sampleMap.sizeY - 1; y++)
                {
                    for (int x = 1; x < sampleMap.sizeX - 1; x++)
                    {
                        if (layer.GetTile(new Vector3Int(x + sampleMap.sampleBounds.xMin - 1, y + sampleMap.sampleBounds.yMin - 1, 0)) != null)
                        {
                            ushort tileId = sampleMap.GetTileId(x, y); if ((tiles[tileId].autoSpawnLimit != prefab.autoSpawnLimit) && (tiles[tileId].autoSpawnLimit >= 0)) Debug.Log("Warning : Conflicting tile auto-spawn-limits found in sample '" + sampleMap.name + "'");
                            tiles[tileId].autoSpawnLimit = prefab.autoSpawnLimit;

                            compPrefab.tileLocations.Add(new Vector3Int(x, y, tileId));
                        }
                    }
                }

                if (compPrefab.tileLocations.Count > 0) compPrefab.handleLocation = new Vector2Int(compPrefab.tileLocations[0].x, compPrefab.tileLocations[0].y);
                compositePrefabs.Add(compPrefab);
            }
        }

        Debug.Log("Found " + compositePrefabs.Count.ToString() + " prefabs in map '" + sampleGrid.name + "'");
    }

    // calculate any composite tile adjacency rules from the sample maps and store those rules as adjacency bitfields in the associated composite tile object
    public void BuildAdjacencyConstraints()
    {
        int tilesCount = tiles.Count;
        for (int t = 0; t < tilesCount; t++)
        {
            for (int a = 0; a < 8; a++)
            {
                tiles[t].adjacents[a] = new AdjacencyList();
                tiles[t].adjacents[a].adjacents = new List<ushort>();
                tiles[t].adjacents[a].tempAdjacentWeights = new Dictionary<ushort, uint>();
            }
        }

        // look at each neighboring tile and construct the adjacency constraints from all sample maps
        foreach (CompositeMap map in sampleMaps)
        {
            for (int y = 0; y < map.sizeY; y++)
            {
                for (int x = 0; x < map.sizeX; x++)
                {
                    ushort tileId = map.GetTileId(x, y);

                    if (tileId < tiles.Count)
                    {
                        for (int a = 0; a < 8; a++)
                        {
                            ushort otherTileId = map.GetTileId(x + CompositeTile.adjacencyOffsets[a].x, y + CompositeTile.adjacencyOffsets[a].y);
                            if (otherTileId < tiles.Count)
                            {
                                if ((tileId != 0) && (otherTileId == 0)) continue;
                                if (!tiles[tileId].adjacents[a].adjacents.Contains(otherTileId)) tiles[tileId].adjacents[a].adjacents.Add(otherTileId);

                                if (tiles[tileId].adjacents[a].tempAdjacentWeights.ContainsKey(otherTileId)) tiles[tileId].adjacents[a].tempAdjacentWeights[otherTileId]++;
                                else tiles[tileId].adjacents[a].tempAdjacentWeights[otherTileId] = 1;
                            }
                        }
                    }
                }
            }

            Debug.Log("Using " + tiles.Count.ToString() + " unique composite tile(s). Using " + numTotalContextBlocks.ToString() + " total contexts");
        }

        for (int t = 0; t < tilesCount; t++)
        {
            for (int a = 0; a < 8; a++)
            {
                tiles[t].adjacents[a].adjacents.Sort();
                tiles[t].adjacents[a].adjacentWeights = new uint[tiles[t].adjacents[a].adjacents.Count];

                uint totalWeight = 0; for (int i = 0; i < tiles[t].adjacents[a].adjacentWeights.Length; i++) totalWeight += tiles[t].adjacents[a].tempAdjacentWeights[tiles[t].adjacents[a].adjacents[i]];
                double scale = 1024.0; //if (totalWeight > 0) scale /= Math.Sqrt(totalWeight);  // dividing by the totalweight here makes mathematical sense but results in objectively worse maps, especially higher temp maps
                for (int i = 0; i < tiles[t].adjacents[a].adjacentWeights.Length; i++)
                {
                    uint weight = (uint)(tiles[t].adjacents[a].tempAdjacentWeights[tiles[t].adjacents[a].adjacents[i]] * scale); if (weight == 0) weight = 1;
                    tiles[t].adjacents[a].adjacentWeights[i] = weight;
                }
                tiles[t].adjacents[a].tempAdjacentWeights.Clear();
            }
        }
    }

    // create and copy the generator data to the native plugin object
    public void InitGenerator()
    {
        if (generator == 0)
        {
            generator = RMX_CreateGenerator(tiles.Count, sampleMaps.Count);

            for (int t = 0; t < tiles.Count; t++)
            {
                for (int a = 0; a < 8; a++)
                {
                    if (tiles[t].adjacents[a].adjacents.Count > 0)
                    {
                        ushort[] tempArray = tiles[t].adjacents[a].adjacents.ToArray();
                        unsafe
                        {
                            fixed (ushort* adjacentIds = &tempArray[0])
                            {
                                fixed (uint* adjacentWeights = &tiles[t].adjacents[a].adjacentWeights[0]) { RMX_Generator_SetTileAdjacency(generator, t, a, tempArray.Length, adjacentIds, adjacentWeights); }
                            }
                        }
                    }
                    else
                    {
                        unsafe { RMX_Generator_SetTileAdjacency(generator, t, a, 0, null, null); }
                    }
                }

                RMX_Generator_SetTileFlags(generator, t, tiles[t].flags);

                if (t > 0)
                {
                    Vector3Int[] tempArray = tiles[t].contextBlocks.ToArray();
                    unsafe
                    {
                        fixed (Vector3Int* locations = &tempArray[0]) { RMX_Generator_SetTileLocations(generator, t, tempArray.Length, locations, tiles[t].autoSpawnLimit); }
                    }
                }
            }

            for (int i = 0; i < sampleMaps.Count; i++)
            {
                unsafe
                {
                    fixed (ushort* mapData = &sampleMaps[i].map[0]) { RMX_Generator_SetSampleMap(generator, i, sampleMaps[i].sizeX, sampleMaps[i].sizeY, mapData); }
                }
            }
        }
    }

    // resets generator and forgets all samples / tiles
    public void ResetToEmpty()
    {
        tiles.Clear();
        sampleMaps.Clear();
        compositePrefabs.Clear();

        numTotalContextBlocks = 0;
        numUniqueTiles = 0;
    }

    // draws an outline around the selected cell in the target grid
    private static void DrawDebugCellHighlight(Grid targetGrid, Vector3Int pos, Color color)
    {
        Vector3 center = (targetGrid.CellToWorld(pos) + targetGrid.CellToWorld(pos + new Vector3Int(1, 1, 0))) * 0.5f;
        Vector3 size = targetGrid.CellToWorld(new Vector3Int(1, 1, 0));

        Gizmos.color = color;
        Gizmos.DrawCube(center, size);
    }

    public void ShowTileDistributionStats()
    {
        List<Vector2Int> dist = new List<Vector2Int>();
        for (int c = 0; c < tiles.Count; c++) dist.Add(new Vector2Int(c, tiles[c].contextBlocks.Count));

        dist.Sort((x, y) => y.y.CompareTo(x.y));

        using (var writer = new BinaryWriter(new FileStream("debug_distribution.raw", FileMode.Create)))
        {
            foreach (var inner in dist) writer.Write(inner.y);
            writer.Close();
        }

        for (int c = 0; c < dist.Count; c++)
        {
            Debug.Log((dist[c].x) + " : " + (dist[c].y));
        }
    }
}

// editor integration for compiled prefab export
#if UNITY_EDITOR
[CustomEditor(typeof(RemixMapGenerator))]
public class remixMapGeneratorEditor : Editor
{
    public override void OnInspectorGUI()
    {
        DrawDefaultInspector();

        if (PrefabUtility.GetPrefabType(target) == PrefabType.None)
        {
            GUILayout.Label("Utility Functions:");
            GUILayout.BeginHorizontal();

            if (GUILayout.Button("Merge selected grids"))
            {
                RemixMapGenerator generator = (RemixMapGenerator)target;

                if (generator.mergeSelectedGrids.Length == 0) Debug.Log("No grid is selected to merge.");
                else
                {
                    for (int i = 0; i < generator.mergeSelectedGrids.Length; i++)
                    {
                        Grid mergeGrid = generator.mergeSelectedGrids[i];
                        if (mergeGrid != null)
                        {
                            if (generator.MergeSample(mergeGrid))
                            {
                                Debug.Log("Successfully merged '" + mergeGrid.gameObject.name + "'");
                                UnityEngine.Object.DestroyImmediate(mergeGrid.gameObject);
                                generator.mergeSelectedGrids[i] = null;
                            }
                            else
                            {
                                Debug.Log("Failed to merge '" + mergeGrid.gameObject.name + "'");
                            }
                        }
                    }
                }
            }

            GUILayout.EndHorizontal();

            GUILayout.Label("Compile Functions:");
            GUILayout.BeginHorizontal();

            if (GUILayout.Button("Select Export Path"))
            {
                RemixMapGenerator generator = (RemixMapGenerator)target;
                generator.exportPath = EditorUtility.SaveFilePanelInProject("Save Prefab", UnityEngine.SceneManagement.SceneManager.GetActiveScene().name, "prefab", "");
            }

            if (GUILayout.Button("Compile & Export to Prefab"))
            {
                RemixMapGenerator generator = (RemixMapGenerator)target;
                if (generator.exportPath == "") generator.exportPath = EditorUtility.SaveFilePanelInProject("Save Prefab", generator.gameObject.name, "prefab", ""); // show path picker if none selected

                if (generator.exportPath != "") // dialog returns empty if cancelled
                {
                    generator.ResetToEmpty();

                    // compile all maps in scene
                    Grid[] samples = (Grid[])GameObject.FindObjectsOfType(typeof(Grid));

                    foreach (Grid sample in samples)
                    {
                        if (sample.name.IndexOf("rmxIgnore", StringComparison.CurrentCultureIgnoreCase) == -1) generator.CompileSample(sample); // if you put rmxIgnore somewhere in the name of the Grid the map is ignored in compilation
                    }

                    // precalculate adjacency
                    generator.BuildAdjacencyConstraints();

                    generator = UnityEngine.Object.Instantiate(generator); // create a copy so we can clear all the tilemaps before saving as a prefab
                    Tilemap[] tilemaps = generator.GetComponentsInChildren<Tilemap>();
                    foreach (Tilemap tilemap in tilemaps)
                    {
                        tilemap.ClearAllTiles();
                    }


                    // export the prefab to the path specified
                    RemixMapGenerator existingPrefab = (RemixMapGenerator)AssetDatabase.LoadAssetAtPath(generator.exportPath, typeof(RemixMapGenerator));
                    if (existingPrefab == null)
                    {
                        PrefabUtility.CreatePrefab(generator.exportPath, generator.gameObject);
                        Debug.Log("Successfully exported prefab to '" + generator.exportPath + "'");
                    }
                    else
                    {
                        PrefabUtility.ReplacePrefab(generator.gameObject, existingPrefab, ReplacePrefabOptions.ReplaceNameBased);
                        Debug.Log("Successfully updated prefab '" + generator.exportPath + "'");
                    }

                    UnityEngine.Object.DestroyImmediate(generator.gameObject);

                    // mark scene as dirty to allow the editor to save the generator properties in the actual scene
                    UnityEditor.SceneManagement.EditorSceneManager.MarkSceneDirty(UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene());
                }
            }

            GUILayout.EndHorizontal();
        }
    }
}
#endif