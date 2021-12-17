using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Tilemaps;

#if UNITY_EDITOR
using UnityEditor;
#endif

public class RemixMap : MonoBehaviour
{
    [Range(1, 65535)]
    public int targetWidth = 100;        // desired map dimensions
    [Range(1, 65535)]
    public int targetHeight = 100;
    public ulong targetSeed = 0;          // desired map seed, 0 for random seed

    public bool useDistributionBias = false;   // bias tile selection slightly to approach total overall tile distribution from sample maps. has more effect for higher temperature generation
    [Range(0, 65535)]
    public int maxPropDensity = 0;
    [Range(1, 100)]
    public int maxSnapshotsToKeep = 5;
    [Range(1, 65535)]
    public int maxTilesBeforeSnapshot = 3000;
    [Range(0, 255)]
    public int randomTemperature = 10;      // 0 is the minimum setting, resulting in a map similar to the actual samples, higher values make more chaotic maps
    [Range(-10000, 10000)]
    public float localityBias = 0.0f;
    public bool prepaintBorder = true;      // if true paint a 1 tile thick layer of OOB tiles around the edge of the map and take a snapshot before starting generation

    public int generatorStepsPerFrame = 250;    // for previewing generation in realtime. higher values mean faster generation but less fps
    public int maxContinuesBeforeRestart = 100; // if the evaluator is unable to fix the map in the specified number of tries then the whole map is restarted

    public bool drawSceneDebugDensity = false;
    public bool drawSceneDebugContexts = false;
    public bool drawSceneDebugLastFailPos = false;
    public bool drawSceneDebugRooms = false;
    public bool drawSceneDebugPathNodes = false;
    public bool drawSceneDebugProgress = false;

    private RemixMapGenerator.SuperMap superMap = null;   // the current map being generated, contains both a CompositeMap and a map of SuperTiles used for constraint calculations and to find next generation position
    private Grid targetGrid = null;                       // the grid component in this game object we are generating into
    private RemixMapGenerator generator = null;           // the actual generator object
    private bool finished = false;

    private Room[] roomMap = null;
    private Region[] regionMap = null;
    private List<Room> rooms = new List<Room>();
    private List<Region> regions = new List<Region>();
    private List<PathNode> pathNodes = new List<PathNode>();

    private RemixEvaluator evaluator = null;

    private int numContinues = 0;
    private int numRestarts = 0;

    public class PathNode
    {
        public Room room = null;
        public Vector2 pos;
        public List<PathNode> links = new List<PathNode>();
    }

    public class Region
    {
        public List<Room> rooms = new List<Room>();
        public int index;
        public bool isCollider = false;
        public uint userFlags = 0;

        public Region(int newIndex)
        {
            index = newIndex;
        }
    }

    public class Room
    {
        public List<Vector2Int> tiles = new List<Vector2Int>();
        public List<Room> adjacents = new List<Room>();
        public List<PathNode> pathNodes = new List<PathNode>();
        public Region region = null;

        public BoundsInt bounds = new BoundsInt();
        public int index;
        public uint userFlags = 0;

        public enum RoomType { Open = 1, StaticCollider = 2, DynamicCollider = 4 };

        public RoomType type = RoomType.Open;

        public Vector2Int GetCenter()
        {
            Vector3 boundsCenter = bounds.center;
            return new Vector2Int((int)boundsCenter.x, (int)boundsCenter.y);
        }
    }

    public void EnableTilemapColliders(bool enabled)
    {
        TilemapCollider2D[] tilemapColliders = GetComponentsInChildren<TilemapCollider2D>(true);
        foreach (TilemapCollider2D tilemapCollider in tilemapColliders) tilemapCollider.enabled = enabled;
    }

    public void EnableTilemapRenderers(bool enabled)
    {
        TilemapRenderer[] tilemapRenderers = GetComponentsInChildren<TilemapRenderer>(true);
        foreach (TilemapRenderer tilemapRenderer in tilemapRenderers) tilemapRenderer.enabled = enabled;
    }
    
    public void CreateNewMap()
    {
        EnableTilemapColliders(false);
        EnableTilemapRenderers(drawSceneDebugProgress);

        superMap = new RemixMapGenerator.SuperMap(generator, targetWidth, targetHeight, targetGrid, targetSeed);
        superMap.SetParams(maxSnapshotsToKeep, maxTilesBeforeSnapshot, maxPropDensity, useDistributionBias);

        if (prepaintBorder) superMap.PaintBorder();
        if (drawSceneDebugProgress) superMap.PaintTarget();
        finished = false;

        evaluator = GetComponent<RemixEvaluator>();
        if (evaluator == null) evaluator = gameObject.AddComponent<RemixEvaluator>();
        evaluator.PrepaintMap(this);
        numContinues = 0;

        superMap.TakeSnapshot(false);
    }

    public int GetRegionTotalArea(int regionIndex)
    {
        if ((regionIndex < 0) || (regionIndex >= regions.Count)) return -1;
        Region region = regions[regionIndex];

        int area = 0, roomCount = region.rooms.Count;
        for (int r = 0; r < roomCount; r++) area += region.rooms[r].tiles.Count;
        
        return area;
    }

    public int GetNumContinues()
    {
        return numContinues;
    }

    public int GetNumRestarts()
    {
        return numRestarts;
    }

    public Grid GetTargetGrid()
    {
        return targetGrid;
    }

    public List<Vector2Int> GetAllRegionTileLocations(int regionIndex)
    {
        if ((regionIndex < 0) || (regionIndex >= regions.Count)) return null;
        Region region = regions[regionIndex];

        List<Vector2Int> tiles = new List<Vector2Int>();
        int roomCount = region.rooms.Count;
        for (int r = 0; r < roomCount; r++) tiles.AddRange(region.rooms[r].tiles);
        
        return tiles;
    }

    public Region[] FindNearbyRegions(List<Vector2Int> searchTiles, int radius, bool findColliders = false)
    {
        Dictionary<Region, bool> addedRegions = new Dictionary<Region, bool>();
        List<Region> returnRegions = new List<Region>();

        int tileCount = searchTiles.Count;
        for (int t = 0; t < tileCount; t++)
        {
            for (int y = searchTiles[t].y - radius; y <= searchTiles[t].y + radius; y++)
            {
                for (int x = searchTiles[t].x - radius; x <= searchTiles[t].x + radius; x++)
                {
                    Region region = GetRegion(x, y);
                    if (region != null)
                    {
                        if (region.isCollider == findColliders)
                        {
                            if (!addedRegions.ContainsKey(region))
                            {
                                addedRegions[region] = true;
                                returnRegions.Add(region);
                            }
                        }
                    }
                }
            }
        }

        return returnRegions.ToArray();
    }

    public List<Vector2Int> GetAllRegionEdgeLocations(int regionIndex)
    {
        if ((regionIndex < 0) || (regionIndex >= regions.Count)) return null;
        Region region = regions[regionIndex];

        Vector2Int[] adjacencyOffsets = { new Vector2Int { x = -1, y = -1 },
                                          new Vector2Int { x =  0, y = -1 },
                                          new Vector2Int { x =  1, y = -1 },
                                          new Vector2Int { x = -1, y =  0 },
                                          new Vector2Int { x =  1, y =  0 },
                                          new Vector2Int { x = -1, y =  1 },
                                          new Vector2Int { x =  0, y =  1 },
                                          new Vector2Int { x =  1, y =  1 }};

        List<Vector2Int> tiles = new List<Vector2Int>();
        int roomCount = region.rooms.Count;
        for (int r = 0; r < roomCount; r++)
        {
            int tileCount = region.rooms[r].tiles.Count;
            for (int t = 0; t < tileCount; t++)
            {
                for (int a = 0; a < adjacencyOffsets.Length; a++)
                {
                    Vector2Int adj = region.rooms[r].tiles[t] + adjacencyOffsets[a];
                    if ((adj.x >= 0) && (adj.x < targetWidth) && (adj.y > 0) && (adj.y < targetHeight))
                    {
                        if (regionMap[adj.y * targetWidth + adj.x] != region)
                        {
                            tiles.Add(region.rooms[r].tiles[t]);
                            break;
                        }
                    }
                }
            }
        }
        return tiles;
    }

    public List<Vector2Int> ExcludeTileLocations(List<Vector2Int> locations, List<Vector2Int> exclude)
    {
        Dictionary<Vector2Int, bool> excludeDict = new Dictionary<Vector2Int, bool>();
        int excludeCount = exclude.Count;
        for (int i = 0; i < excludeCount; i++) excludeDict[exclude[i]] = true;

        List<Vector2Int> filteredLocations = new List<Vector2Int>();

        int locationCount = locations.Count;
        for (int i = 0; i < locationCount; i++) if (!excludeDict.ContainsKey(locations[i])) filteredLocations.Add(locations[i]);
        return filteredLocations;
    }

    public List<Vector2Int> GetRect(int x, int y, int sizeX, int sizeY)
    {
        List<Vector2Int> rect = new List<Vector2Int>();

        if (x < 0) { sizeX += x; x = 0; }
        if (y < 0) { sizeY += y; y = 0; }
        if ((x + sizeX) > targetWidth) sizeX = targetWidth - x;
        if ((y + sizeY) > targetHeight) sizeY = targetHeight - y;

        for (int ty = y; ty < (y + sizeY); ty++)
        {
            for (int tx = x; tx < (x + sizeX); tx++)
            {
                rect.Add(new Vector2Int(tx, ty));
            }
        }

        return rect;
    }

    public List<Vector2Int> GetLine(Vector2Int from, Vector2Int to, int thickness = 1, bool ignoreRoomTiles = false)
    {
        List<Vector2Int> line = new List<Vector2Int>();        
        if (--thickness < 0) return line;
        //int squareRadius = thickness * thickness;

        Dictionary<Vector2Int, bool> lineDict = new Dictionary<Vector2Int, bool>();

        int x = from.x, y = from.y;
        int dx = to.x - from.x, dy = to.y - from.y;

        bool inverted = false;
        int step = Math.Sign(dx);
        int gradientStep = Math.Sign(dy);

        int longest = Mathf.Abs(dx);
        int shortest = Mathf.Abs(dy);

        if (longest < shortest)
        {
            inverted = true;
            longest = Mathf.Abs(dy);
            shortest = Mathf.Abs(dx);

            step = Math.Sign(dy);
            gradientStep = Math.Sign(dx);
        }

        int gradientAccumulation = longest / 2;
        for (int i = 0; i < longest; i++)
        {
            for (int ty = y - thickness; ty <= (y + thickness); ty++)
            {
                for (int tx = x - thickness; tx <= (x + thickness); tx++)
                {
                    if ((tx < 0) || (ty < 0) || (tx >= targetWidth) || (ty >= targetHeight)) continue;
                    if (ignoreRoomTiles && (roomMap[ty * targetWidth + tx] != null)) continue;

                    //int squareDist = (tx - x) * (tx - x) + (ty - y) * (ty - y);
                    //if (squareDist <= squareRadius)
                    {
                        Vector2Int loc = new Vector2Int(tx, ty);
                        if (!lineDict.ContainsKey(loc))
                        {
                            lineDict[loc] = true;
                            line.Add(loc);
                        }
                    }
                }
            }

            if (inverted) y += step;
            else x += step;

            gradientAccumulation += shortest;
            if (gradientAccumulation >= longest)
            {
                if (inverted) x += gradientStep;
                else y += gradientStep;
                
                gradientAccumulation -= longest;
            }
        }

        return line;
    }

    public RemixMapGenerator.CompositePrefab GetPrefab(string name)
    {
        return generator.FindPrefabByName(name);
    }

    public RemixMapGenerator.SuperMap GetSuperMap()
    {
        return superMap;
    }

    public Room[] GetRooms(Room.RoomType type = Room.RoomType.Open)
    {
        List<Room> returnRooms = new List<Room>();
        int roomCount = rooms.Count;
        for (int r = 0; r < roomCount; r++) if (rooms[r].type == type) returnRooms.Add(rooms[r]);

        return returnRooms.ToArray();
    }

    public Region[] GetRegions(bool getColliders = false)
    {
        List<Region> returnRegions = new List<Region>();
        int regionCount = regions.Count;
        for (int r = 0; r < regionCount; r++) if (regions[r].isCollider == getColliders) returnRegions.Add(regions[r]);

        return returnRegions.ToArray();
    }

    public bool AreRoomsConnected(int roomIndex1, int roomIndex2, uint userFlagsFilter = uint.MaxValue) // returns whether 2 rooms are connected by adjacency, directly or indirectly, avoiding rooms that do not match the specified user flag mask
    {
        int roomCount = rooms.Count;
        if ((roomIndex1 < 0) || (roomIndex1 >= roomCount) || (roomIndex2 < 0) || (roomIndex2 >= roomCount)) return false;

        if (roomIndex1 == roomIndex2) return true;
        if (userFlagsFilter == uint.MaxValue) return rooms[roomIndex1].region.rooms.Contains(rooms[roomIndex2]);

        Dictionary<int, bool> checkedRooms = new Dictionary<int, bool>();
        List<int> roomsToCheck = new List<int>();
        int currentRoom = roomIndex1; checkedRooms[currentRoom] = true;
        
        while (true)
        {
            int adjacentCount = rooms[currentRoom].adjacents.Count;
            for (int i = 0; i < adjacentCount; i++)
            {
                int adjacentRoomIndex = rooms[currentRoom].adjacents[i].index;
                if (adjacentRoomIndex == roomIndex2)
                {
                    return true;
                }
                else
                {
                    if (!checkedRooms.ContainsKey(adjacentRoomIndex))
                    {
                        checkedRooms[adjacentRoomIndex] = true;
                        if ((rooms[adjacentRoomIndex].userFlags & userFlagsFilter) != 0) roomsToCheck.Add(adjacentRoomIndex);
                    }
                }
            }

            if (roomsToCheck.Count == 0)
            {
                break;
            }
            else
            {
                currentRoom = roomsToCheck[roomsToCheck.Count - 1];
                roomsToCheck.RemoveAt(roomsToCheck.Count - 1);
            }
        }

        return false;
    }

    public Vector2Int[] FindClosestTileLocations(List<Vector2Int> tileList1, List<Vector2Int> tileList2)
    {
        Vector2Int[] closest = new Vector2Int[2];
        long lowestDist = long.MaxValue;

        int tileCount1 = tileList1.Count, tileCount2 = tileList2.Count;
        for (int t1 = 0; t1 < tileCount1; t1++)
        {        
            for (int t2 = 0; t2 < tileCount2; t2++)
            {
                long dist = (tileList1[t1].x - tileList2[t2].x) * (tileList1[t1].x - tileList2[t2].x) +
                            (tileList1[t1].y - tileList2[t2].y) * (tileList1[t1].y - tileList2[t2].y);

                if (dist < lowestDist)
                {
                    lowestDist = dist;
                    closest[0] = tileList1[t1];
                    closest[1] = tileList2[t2];
                    if (dist <= 1) return closest;
                }
            }
        }

        if (lowestDist == long.MaxValue) return null;
        return closest;
    }

    public List<Vector2Int> FindPrefabLocations(RemixMapGenerator.CompositePrefab prefab, bool findAnyPrefabTile = false)
    {
        List<Vector2Int> prefabLocations = new List<Vector2Int>();
        int numPrefabTiles = prefab.tileLocations.Count;

        if (findAnyPrefabTile)
        {
            for (int y = 0; y < targetHeight; y++)
            {
                for (int x = 0; x < targetWidth; x++)
                {            
                    for (int t = 0; t < numPrefabTiles; t++)
                    {
                        ushort tileId = (ushort)prefab.tileLocations[t].z;
                        if (superMap.targetMap.GetTileId(x, y) == tileId)
                        {
                            prefabLocations.Add(new Vector2Int(x, y));
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            for (int y = 0; y < targetHeight; y++)
            {
                for (int x = 0; x < targetWidth; x++)
                {
                    bool prefabFound = true;

                    for (int t = 0; t < numPrefabTiles; t++)
                    {
                        int tx = prefab.tileLocations[t].x - prefab.handleLocation.x + x;
                        int ty = prefab.tileLocations[t].y - prefab.handleLocation.x + y;
                        if ((tx < 0) || (ty < 0) || (tx >= targetWidth) || (ty >= targetHeight))
                        {
                            prefabFound = false;
                            break;
                        }
                        ushort tileId = (ushort)prefab.tileLocations[t].z;
                        
                        if (superMap.targetMap.GetTileId(tx, ty) != tileId)
                        {
                            prefabFound = false;
                            break;
                        }
                    }

                    if (prefabFound) prefabLocations.Add(new Vector2Int(x, y));
                }
            }
        }

        return prefabLocations;
    }

    public Room GetRoom(int x, int y)
    {
        if (roomMap == null) return null;
        if ((x < 0) || (y < 0) || (x >= targetWidth) || (y >= targetHeight)) return null;
        return roomMap[y * targetWidth + x];
    }

    public Region GetRegion(int x, int y)
    {
        if (regionMap == null) return null;
        if ((x < 0) || (y < 0) || (x >= targetWidth) || (y >= targetHeight)) return null;
        return regionMap[y * targetWidth + x];
    }

    void Start()
    {
        generator = gameObject.GetComponent<RemixMapGenerator>();
        targetGrid = gameObject.GetComponentInChildren<Grid>();

        //CreateNewMap();
    }

    void DrawDebugRooms()
    {
        int lastIndex = -1;
        Color lastColor = new Color(0.0f, 0.0f, 0.0f, 1.0f);
        System.Random rng = new System.Random();

        foreach (Room room in rooms)
        {
            int roomIndex = room.index;
            if (roomIndex != lastIndex)
            {
                lastIndex = roomIndex;
                rng = new System.Random(roomIndex);
                lastColor = new Color((float)rng.NextDouble(), (float)rng.NextDouble(), (float)rng.NextDouble(), 0.75f);
            }

            int roomTileCount = room.tiles.Count;
            for (int i = 0; i < roomTileCount; i++) DrawDebugCellHighlight(targetGrid, new Vector3Int(room.tiles[i].x, room.tiles[i].y, 0), lastColor);
        }
    }

    public void BuildRoomsAndRegions()
    {
        EnableTilemapColliders(true);
        
        rooms.Clear();
        regions.Clear();

        Vector2Int[] roomAdjacencyOffsets = { new Vector2Int { x =  0, y = -1 },
                                              new Vector2Int { x =  0, y =  1 },
                                              new Vector2Int { x = -1, y =  0 },
                                              new Vector2Int { x =  1, y =  0 } };

        float contactOffset = Physics2D.defaultContactOffset * 4.5f;
        Vector2 occluderSize = new Vector2(0.5f * targetGrid.cellSize.x - contactOffset, 0.5f * targetGrid.cellSize.y - contactOffset);

        roomMap = new Room[targetWidth * targetHeight];
        regionMap = new Region[targetWidth * targetHeight];

        Physics2D.queriesStartInColliders = false;

        for (int y = 0; y < targetHeight; y++)
        {
            for (int x = 0; x < targetWidth; x++)
            {
                if (superMap.targetMap.map[y * targetWidth + x] == 0) continue;

                Vector2 center = (targetGrid.CellToWorld(new Vector3Int(x, y, 0)) + targetGrid.CellToWorld(new Vector3Int(x + 1, y + 1, 0))) * 0.5f;
                Collider2D staticCollider  = Physics2D.OverlapBox(center, occluderSize, 0.0f, generator.staticBlockerLayers);
                Collider2D dynamicCollider = Physics2D.OverlapBox(center, occluderSize, 0.0f, generator.dynamicBlockerLayers);

                Room.RoomType roomType = Room.RoomType.Open;
                if (dynamicCollider != null) roomType = Room.RoomType.DynamicCollider;
                if (staticCollider != null) roomType = Room.RoomType.StaticCollider;
                Room room = null;

                for (int a = 0; a < 4; a++)
                {
                    Vector2Int adj = roomAdjacencyOffsets[a]; adj.x += x; adj.y += y;
                    if ((adj.x < 0) || (adj.x >= targetWidth) || (adj.y < 0) || (adj.y >= targetHeight)) continue;

                    Room adjRoom = roomMap[adj.y * targetWidth + adj.x];
                    if ((adjRoom != null) && (room != adjRoom))
                    {
                        if (adjRoom.type != roomType) continue;

                        if (roomType == Room.RoomType.Open)
                        {
                            Vector2 direction = targetGrid.CellToWorld(new Vector3Int(roomAdjacencyOffsets[a].x, roomAdjacencyOffsets[a].y, 0));
                            RaycastHit2D hit = Physics2D.BoxCast(center, occluderSize, 0.0f, direction, direction.magnitude, generator.staticBlockerLayers | generator.dynamicBlockerLayers);
                            //Debug.DrawRay(center, direction, new Color(1.0f, 0.0f, 0.0f, 1.0f), 15.0f);
                            if (hit.collider != null) continue;
                        }
                        
                        if (room != null)
                        {
                            int adjRoomTileCount = adjRoom.tiles.Count;
                            for (int i = 0; i < adjRoomTileCount; i++) roomMap[adjRoom.tiles[i].y * targetWidth + adjRoom.tiles[i].x] = room;
                            room.tiles.AddRange(adjRoom.tiles);

                            rooms.Remove(adjRoom);
                        }
                        else
                        {
                            room = roomMap[adj.y * targetWidth + adj.x];
                        }                            
                    }
                }
                if (room == null)
                {
                    room = new Room();
                    room.type = roomType;
                    rooms.Add(room);
                }

                room.tiles.Add(new Vector2Int(x, y));
                roomMap[y * targetWidth + x] = room;
            }            
        }

        int roomCount = rooms.Count;
        for (int r = 0; r < roomCount; r++)
        {
            rooms[r].index = r;

            int tileCount = rooms[r].tiles.Count;
            if (tileCount > 0)
            {
                Vector3Int firstTilePos = new Vector3Int(rooms[r].tiles[0].x, rooms[r].tiles[0].y, 0);
                rooms[r].bounds.SetMinMax(firstTilePos, firstTilePos);
            }

            for (int t = 0; t < tileCount; t++)
            {
                if (rooms[r].tiles[t].x < rooms[r].bounds.xMin) rooms[r].bounds.xMin = rooms[r].tiles[t].x;
                if (rooms[r].tiles[t].y < rooms[r].bounds.yMin) rooms[r].bounds.yMin = rooms[r].tiles[t].y;
                if (rooms[r].tiles[t].x > rooms[r].bounds.xMax) rooms[r].bounds.xMax = rooms[r].tiles[t].x;
                if (rooms[r].tiles[t].y > rooms[r].bounds.yMax) rooms[r].bounds.yMax = rooms[r].tiles[t].y;

                for (int a = 0; a < 4; a++)
                {
                    Vector2Int adj = roomAdjacencyOffsets[a]; adj.x += rooms[r].tiles[t].x; adj.y += rooms[r].tiles[t].y;
                    if ((adj.y < 0) || (adj.y >= targetHeight) || (adj.x < 0) || (adj.x >= targetWidth)) continue;
                    Room adjRoom = roomMap[adj.y * targetWidth + adj.x];
                    if ((adjRoom != null) && (adjRoom != rooms[r]))
                    {
                        if ((adjRoom.type & Room.RoomType.StaticCollider) == (rooms[r].type & Room.RoomType.StaticCollider))
                        {
                            if (!rooms[r].adjacents.Contains(adjRoom)) rooms[r].adjacents.Add(adjRoom);
                            if (!adjRoom.adjacents.Contains(rooms[r])) adjRoom.adjacents.Add(rooms[r]);
                        }
                    }
                }
            }   
        }

        Dictionary<int, bool> checkedRooms = new Dictionary<int, bool>();
        List<int> roomsToCheck = new List<int>();

        for (int roomIndex1 = 0; roomIndex1 < roomCount; roomIndex1++)
        {
            if (rooms[roomIndex1].region == null)
            {
                rooms[roomIndex1].region = new Region(regions.Count);
                rooms[roomIndex1].region.rooms.Add(rooms[roomIndex1]);
                regions.Add(rooms[roomIndex1].region);

                if (rooms[roomIndex1].type != Room.RoomType.Open) rooms[roomIndex1].region.isCollider = true;

                int tileCount = rooms[roomIndex1].tiles.Count;
                for (int i = 0; i < tileCount; i++) regionMap[rooms[roomIndex1].tiles[i].y * targetWidth + rooms[roomIndex1].tiles[i].x] = rooms[roomIndex1].region;
            }

            for (int roomIndex2 = roomIndex1 + 1; roomIndex2 < roomCount; roomIndex2++)
            {
                checkedRooms.Clear(); int currentRoom = roomIndex1; checkedRooms[currentRoom] = true;
                roomsToCheck.Clear();

                bool connected = false;

                while (true)
                {
                    int adjacentCount = rooms[currentRoom].adjacents.Count;
                    for (int i = 0; i < adjacentCount; i++)
                    {
                        int adjacentRoomIndex = rooms[currentRoom].adjacents[i].index;
                        if (adjacentRoomIndex == roomIndex2)
                        {
                            connected = true;
                            break;
                        }
                        else
                        {
                            if (!checkedRooms.ContainsKey(adjacentRoomIndex))
                            {
                                checkedRooms[adjacentRoomIndex] = true;
                                roomsToCheck.Add(adjacentRoomIndex);
                            }
                        }
                    }

                    if (roomsToCheck.Count == 0)
                    {
                        break;
                    }
                    else
                    {
                        currentRoom = roomsToCheck[roomsToCheck.Count - 1];
                        roomsToCheck.RemoveAt(roomsToCheck.Count - 1);
                    }
                }

                if (connected)
                {
                    if (rooms[roomIndex2].region != rooms[roomIndex1].region)
                    {
                        rooms[roomIndex2].region = rooms[roomIndex1].region;
                        rooms[roomIndex2].region.rooms.Add(rooms[roomIndex2]);

                        int tileCount = rooms[roomIndex2].tiles.Count;
                        for (int i = 0; i < tileCount; i++) regionMap[rooms[roomIndex2].tiles[i].y * targetWidth + rooms[roomIndex2].tiles[i].x] = rooms[roomIndex2].region;
                    }
                }
            }
        }

        EnableTilemapColliders(false);
    }

    public void ClearRoom(int roomIndex)
    {
        if ((roomIndex < 0) || (roomIndex >= rooms.Count)) return;
        Room room = rooms[roomIndex];
        superMap.ClearTiles(room.tiles.ToArray(), true, true, false);

        int roomCount = rooms.Count; for (int i = 0; i < roomCount; i++) rooms[i].adjacents.Remove(room);
        if (room.region != null)
        {
            room.region.rooms.Remove(room);
            room.region = null;
        }

        int tileCount = room.tiles.Count;
        for (int i = 0; i < tileCount; i++)
        {
            roomMap[room.tiles[i].y * targetWidth + room.tiles[i].x] = null;
            regionMap[room.tiles[i].y * targetWidth + room.tiles[i].x] = null;
        }

        room.userFlags = 0;
        room.tiles.Clear();
        room.adjacents.Clear();
        room.bounds = new BoundsInt();
    }

    public void ClearRegion(int regionIndex)
    {
        if ((regionIndex < 0) || (regionIndex >= regions.Count)) return;
        Region region = regions[regionIndex];

        Room[] regionRooms = region.rooms.ToArray();
        for (int r = 0; r < regionRooms.Length; r++) ClearRoom(regionRooms[r].index);
    }

    void BuildPathNodes(Vector2 characterSize)
    {

        Vector2Int[] roomAdjacencyOffsets = { new Vector2Int { x =  0, y = -1 },
                                              new Vector2Int { x =  0, y =  1 },
                                              new Vector2Int { x = -1, y =  0 },
                                              new Vector2Int { x =  1, y =  0 } };
                   


        pathNodes.Clear();

        int regionCount = regions.Count;
        for (int r = 0; r < regionCount; r++)
        {
            List<Vector2Int> tiles = GetAllRegionEdgeLocations(r);
            int tileCount = tiles.Count;
            for (int t = 0; t < tileCount; t++)
            {
                if ((tiles[t].x <= 0) || (tiles[t].x >= (targetWidth - 1)) || (tiles[t].y <= 0) || (tiles[t].y >= (targetHeight - 1))) continue;

                Room room = roomMap[tiles[t].y * targetWidth + tiles[t].x];
                Room[] adjRooms = new Room[4];
                for (int a = 0; a < 4; a++)
                {
                    Vector2Int adj = roomAdjacencyOffsets[a] + tiles[t];
                    adjRooms[a] = roomMap[adj.y * targetWidth + adj.x];
                }
                bool isCorner = ((adjRooms[0] != room) && (adjRooms[2] != room)) || ((adjRooms[1] != room) && (adjRooms[3] != room));

                if ((room != null) && isCorner)
                {
                    bool redundant = false;

                    float contactOffset = Physics2D.defaultContactOffset * 4.5f;
                    Vector2 size = new Vector2(characterSize.x * targetGrid.cellSize.x - contactOffset, characterSize.y * targetGrid.cellSize.y - contactOffset);
                    Vector2 center = (targetGrid.CellToWorld(new Vector3Int(tiles[t].x, tiles[t].y, 0)) + targetGrid.CellToWorld(new Vector3Int(tiles[t].x + 1, tiles[t].y + 1, 0))) * 0.5f;

                    foreach (PathNode roomNode in room.pathNodes)
                    {    
                        Vector2 direction = center - roomNode.pos;
                        RaycastHit2D staticHit = Physics2D.BoxCast(roomNode.pos, size, 0.0f, direction, direction.magnitude, generator.staticBlockerLayers);

                        if (staticHit.collider == null)
                        {
                            redundant = true;
                            break;
                        }
                    }

                    if (!redundant)
                    {
                        PathNode node = new PathNode();
                        node.pos = center;
                        pathNodes.Add(node);

                        node.room = room;
                        node.room.pathNodes.Add(node);
                    }
                }
            }
        }

        //Physics2D.queriesStartInColliders = true;

        /*
        int pathNodeCount = pathNodes.Count;
        for (int p1 = 0; p1 < pathNodeCount; p1++)
        {
            for (int p2 = p1 + 1; p2 < pathNodeCount; p2++)
            {
                Vector2 delta = pathNodes[p2].pos - pathNodes[p1].pos;
                RaycastHit2D hit = Physics2D.Raycast(pathNodes[p1].pos, delta, delta.magnitude, staticBlockerLayers);

                if (hit.collider == null)
                {
                    pathNodes[p1].links.Add(pathNodes[p2]);
                    pathNodes[p2].links.Add(pathNodes[p1]);
                }
            }
        }*/
    }

    void DrawDebugPathNodes()
    {
        Gizmos.color = new Color(0.8f, 0.8f, 0.5f, 1.0f);
        int numPathNodes = pathNodes.Count;
        for (int i = 0; i < numPathNodes; i++)
        {
            int linkCount = pathNodes[i].links.Count;
            for (int l = 0; l < linkCount; l++) Gizmos.DrawLine(pathNodes[i].pos, pathNodes[i].links[l].pos);
            Gizmos.DrawCube(pathNodes[i].pos, targetGrid.cellSize * 0.1f);
        }
    }

	void Update()
    {
        // continue generating the map if not finished
        if (!finished)
        {
            if (superMap == null) return;
            if (superMap.GenerateMap(generatorStepsPerFrame, randomTemperature, drawSceneDebugProgress, localityBias))
            {
                if (!drawSceneDebugProgress) superMap.PaintTarget();

                switch (evaluator.EvaluateMap(this))
                {
                    case RemixEvaluator.EvalResult.Restart:
                        numRestarts++;
                        CreateNewMap();
                        break;

                    case RemixEvaluator.EvalResult.Continue:
                        if (++numContinues >= maxContinuesBeforeRestart)
                        {
                            Debug.Log("Warning: Continue limit (" + maxContinuesBeforeRestart.ToString() + ") reached, restarting map generation...");
                            numRestarts++; numContinues = 0;
                            CreateNewMap();
                        }
                        else
                        {
                            numContinues++;
                            superMap.TakeSnapshot(false);
                        }
                        break;

                    case RemixEvaluator.EvalResult.Success:
                        EnableTilemapColliders(true);
                        evaluator.PostprocessMap(this);
                        EnableTilemapRenderers(true);
                        finished = true;
                        break;
                }
            }
        }
    }

    private void OnDrawGizmos()
    {
        if (Application.isEditor)
        {
            if (drawSceneDebugRooms) DrawDebugRooms();
            if (drawSceneDebugPathNodes) DrawDebugPathNodes();

            if (superMap != null)
            {
                if (drawSceneDebugDensity) superMap.DrawDebugDensity();
                if (drawSceneDebugContexts) superMap.DrawDebugContexts();

                if (drawSceneDebugLastFailPos)
                {
                    if ((superMap.lastFailPos.x != -1) && (superMap.lastFailPos.y != -1))
                    {
                        Vector3Int lastFailPos = new Vector3Int(superMap.lastFailPos.x, superMap.lastFailPos.y, 0);
                        Color color = new Color(1.0f, 0.0f, 0.0f, 1.0f);
                        DrawDebugCellHighlight(targetGrid, lastFailPos, color);
                    }
                }
            }
        }
    }

    private static void DrawDebugCellHighlight(Grid targetGrid, Vector3Int pos, Color color)
    {
        Vector3 center = (targetGrid.CellToWorld(pos) + targetGrid.CellToWorld(pos + new Vector3Int(1, 1, 0))) * 0.5f;
        Vector3 size = targetGrid.CellToWorld(new Vector3Int(1, 1, 0));

        Gizmos.color = color;
        Gizmos.DrawCube(center, size);
    }
}

// adds button in inspector while running in editor to export generated map to prefab
#if UNITY_EDITOR
[CustomEditor(typeof(RemixMap))]
public class remixMapEditor : Editor
{
    public override void OnInspectorGUI()
    {
        DrawDefaultInspector();

        if (Application.isPlaying)
        {
            if (GUILayout.Button("Create New Map"))
            {
                RemixMap map = (RemixMap)target;
                map.CreateNewMap();
            }

            if (GUILayout.Button("Export Map to Prefab"))
            {
                RemixMap map = (RemixMap)target;
                string exportPath = EditorUtility.SaveFilePanelInProject("Save Prefab", UnityEngine.SceneManagement.SceneManager.GetActiveScene().name, "prefab", "");

                if (exportPath != "")   // dialog returns empty if cancelled
                {
                    // export the prefab to the path specified
                    RemixMap existingPrefab = (RemixMap)AssetDatabase.LoadAssetAtPath(exportPath, typeof(RemixMap));
                    if (existingPrefab == null)
                    {
                        PrefabUtility.CreatePrefab(exportPath, map.gameObject);
                        Debug.Log("Successfully exported prefab to '" + exportPath + "'");
                    }
                    else
                    {
                        PrefabUtility.ReplacePrefab(map.gameObject, existingPrefab, ReplacePrefabOptions.ReplaceNameBased);
                        Debug.Log("Successfully updated prefab '" + exportPath + "'");
                    }
                }
            }

            if (GUILayout.Button("Clear Current Map"))
            {
                RemixMap map = (RemixMap)target;
                Tilemap[] layers = map.GetComponentsInChildren<Tilemap>();
                for (int l = 0; l < layers.Length; l++) layers[l].ClearAllTiles();
            }
        }
    }
}
#endif
      