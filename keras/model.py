import os

import numba
import numpy as np
import bitset
import lgprocess
import pickle
import copy

DATA_DIR = './data'
LOG_DIR = './logs'
MODEL_DIR = './model'

SEQ_LEN = 768

def get_64bit_str(i):
    return '{0:#066b}'.format(i)

class Link(object):
    
    def __init__(self, model, unitFrom, unitTo, unitOffset):
        self.model = model
        self.unitFrom = unitFrom
        self.unitTo = unitTo
        self.offset = unitOffset
        self.weights = np.zeros((unitFrom.width * 64, unitTo.width * 64), dtype=np.uint64)
        self.mask = np.zeros((unitFrom.width * 64, unitTo.width), dtype=np.uint64)
        self.maskCount = 0
        return
     
    def init_all(self):
        bitset.set_all(self.mask)
        self.maskCount = bitset.count(self.mask)
        self.weights = np.ones((self.unitFrom.width * 64, self.unitTo.width * 64), dtype=np.uint64)
        return
        
    def init_random(self, max_init, mask_threshold):
        self.weights = np.random.randint(low=1, high=max_init, size=(self.unitFrom.width * 64, self.unitTo.width * 64), dtype=np.uint64)
        bitset.threshold_mask(self.mask, self.weights, mask_threshold)
        self.maskCount = bitset.count(self.mask)
        return
        
class UnitInstance(object):

    def __init__(self, model, unit, position):
        self.model = model
        self.unit = unit
        self.state = np.zeros(unit.width, dtype=np.uint64)
        self.dist = np.ones(unit.width * 64, dtype=np.uint64)
        #self.dist = np.ones(unit.width * 64)
        self.instanceToLinks = []
        self.instanceFromLinks = []
        self.count = 0
        self.position = position
        self.val = -1
        self.reset()
        return
    
    def set_val(self, val):
        bitset.set_none(self.state)
        bitset.set_bit(self.state, val)
        self.count = 1
        self.val = val
        return
        
    def reset(self):
        bitset.set_all(self.state)
        self.count = self.unit.width * 64
        self.val = -1
        return
    
    def relink(self):
        self.instanceToLinks = []
        self.instanceFromLinks = []
        for link in self.unit.links:
            linkedPos = tuple(map(lambda x, y: x + y, self.position, link.offset))
            try:
                targetInstance = link.unitTo.instances[linkedPos]
            except:
                continue
            self.instanceToLinks.append((targetInstance, link))
            targetInstance.instanceFromLinks.append((self, link))                            
        return
    
    def rebuild_dist(self):
        mask = np.zeros(len(self.dist), dtype=np.uint64)
        bitset.sum_list(mask, [self.state])
        self.dist[:] = mask[:]
        #self.dist[:] = -20
        for i in range(len(self.instanceFromLinks)):
            otherInstance = self.instanceFromLinks[i][0]
            if otherInstance.val >= 0:
                self.dist += self.instanceFromLinks[i][1].weights[otherInstance.val] * mask * 1000
            #else:
            #    for otherVal in range(len(otherInstance.state) * 64):
            #        if bitset.get_bit(otherInstance.state, otherVal):
            #            self.dist += self.instanceFromLinks[i][1].weights[otherVal] * mask

                
    def collapse(self):
        
        self.rebuild_dist()
        self.set_val(bitset.sample_random(self.dist))
        #self.dist = np.nan_to_num(np.exp(self.dist), 0) + 0.000001
        #self.dist /= np.sum(self.dist)
        #options = np.arange(len(self.state) * 64)
        #self.set_val(np.random.choice(options, p=self.dist))
        
        """
        for i in range(len(self.instanceFromLinks)):
            otherInstance = self.instanceFromLinks[i][0]
            if otherInstance.val >= 0:
                self.instanceFromLinks[i][1].weights[otherInstance.val, self.val] += 1

        for i in range(len(self.instanceToLinks)):
            otherInstance = self.instanceToLinks[i][0]
            if otherInstance.val >= 0:
                self.instanceToLinks[i][1].weights[self.val, otherInstance.val] += 1
        """
        
        return self.val
    
    def copy(self):
        _copy = UnitInstance.__new__(UnitInstance)
        _copy.model = self.model
        _copy.unit = self.unit
        _copy.position = self.position
        _copy.state = self.state.copy()
        _copy.dist = self.dist
        _copy.instanceToLinks = self.instanceToLinks
        _copy.instanceFromLinks = self.instanceFromLinks
        _copy.count = self.count
        _copy.val = self.val
        return _copy
        
class UnitClass(object):

    def __init__(self, model, width, id, name=''):
        self.model = model
        self.id = id
        self.links = []
        self.instances = {}
        self.width = width
        if name == '': name = '_unit{}'.format(id)
        self.name = name
        return
    
    def add_link(self, unitTo, unitOffset):
        link = Link(self.model, self, unitTo, unitOffset)
        self.links.append(link)
        return link
    
    def set_instance_val(self, position, val=-1):
        try:
            instance = self.instances[position]
            if val == -1: instance.reset()
            else: instance.set_val(val)
        except:
            instance = UnitInstance(self.model, self, position)
            if val == -1: instance.reset()
            else: instance.set_val(val)            
            self.instances[position] = instance
        return instance
    
    def get_instance_val(self, position):
        try:
            instance = self.instances[position]
            return instance.val
        except:
            return -1
        
class Model(object):

    def __init__(self):
        self.units = []
        self.snapshots = []
        return

    def summary(self):
        
        print('')
        print('Model Summary:')
        print('')
        
        totalUnits = len(self.units)
        totalLinks = 0
        totalWeights = 0
        totalInstances = 0
        totalSnapshots = len(self.snapshots)
        
        for unit in self.units:
            unitLinks = len(unit.links)
            unitInstances = len(unit.instances)
            
            unitWeights = 0
            for link in unit.links:
                unitWeights += link.weights.size
            
            print('UnitClass:     "{}"'.format(unit.name))
            print('    Width:     {}'.format(unit.width))
            print('    Links:     {}'.format(unitLinks))
            print('    Weights:   {}'.format(unitWeights))
            print('    Instances: {}'.format(unitInstances))
            
            totalLinks += unitLinks
            totalWeights += unitWeights
            totalInstances += unitInstances
        
        print('')
        print('Total Units:     {}'.format(totalUnits))
        print('Total Links:     {}'.format(totalLinks))
        print('Total Weights:   {}'.format(totalWeights))
        print('Total Instances: {}'.format(totalInstances))
        print('Total Snapshots: {}'.format(totalSnapshots))
        print('')
        
        return

    def add_unit_class(self, width, name=''):
        id = len(self.units)
        unit = UnitClass(self, width, id, name)
        self.units.append(unit)
        return unit
    
    def clear_instances(self):
        for unit in self.units: unit.instances = {}
        return
        
    def set_instance_val(self, id, position, val=-1):
        return self.units[id].set_instance_val(position, val)

    def get_instance_val(self, id, position):
        return self.units[id].get_instance_val(position)
        
    def scroll_instances(self, offset, boundsLow, boundsHigh):
        for unit in self.units:
            scrolledInstances = {}
            for position, instance in unit.instances.items():
                scrolledPos = tuple(map(lambda x, y: x + y, position, offset))
                for d in range(len(boundsLow)):
                    if scrolledPos[d] < boundsLow[d]: continue
                    if scrolledPos[d] >= boundsHigh[d]: continue
                instance.position = scrolledPos
                scrolledInstances[scrolledPos] = instance
            unit.instances = scrolledInstances
        self.relink_instances()
        return
    
    def relink_instances(self):
        for unit in self.units:
            for position, instance in unit.instances.items():
                instance.relink()
        return
    
    def clear_snapshots(self):
        self.snapshots = []
        
    def create_snapshot(self):
        snapshot = []
        for unit in self.units:
            instances = {}
            for position, instance in unit.instances.items():
                instances[position] = instance.copy()
            snapshot.append(instances)
        self.snapshots.append(snapshot)
        return snapshot
        
    def restore_snapshot(self):
        assert len(self.snapshots) > 0
        snapshot = self.snapshots[-1]
        for i in range(len(snapshot)):
            self.units[i].instances = {}
            for position, instance in snapshot[i].items():
                self.units[i].instances[position] = instance.copy()

        self.relink_instances()
        return
    
    def constrain(self, instance):
        dirtyInstances = [instance]
        while len(dirtyInstances) > 0:
            instance = dirtyInstances[-1]
            
            for i in range(len(instance.instanceToLinks)):
                otherInstance = instance.instanceToLinks[i][0]
                
                if otherInstance.val < 0:
                    scratch = np.zeros(otherInstance.unit.width, dtype=np.uint64)
                    bitset.select_merge(scratch, instance.instanceToLinks[i][1].mask, instance.state)
                    
                    oldCount = otherInstance.count
                    otherInstance.count = bitset.bitwise_and_count(otherInstance.state, scratch)
                    
                    if otherInstance.count == 0:
                        return False
                    if oldCount > otherInstance.count:
                        if not (otherInstance in dirtyInstances): dirtyInstances.append(otherInstance)
                        
            dirtyInstances.remove(instance)

        return True
        
    def find_next_target_instance(self):
        lowestCountInstance = None
        lowestCount = 1 << 30
        
        for unit in self.units:
            for position, instance in unit.instances.items():
                if instance.val == -1 and instance.count < lowestCount:
                    lowestCount = instance.count
                    lowestCountInstance = instance
        return lowestCountInstance
        
    def solve(self, max_restarts=1000, snapshot_freq=250, no_constrain=False):
        
        self.clear_snapshots()
        self.create_snapshot()

        restarts = 0
        steps = 0
        nextInstance = self.find_next_target_instance()
        
        while nextInstance != None:
        
            steps += 1
            if (steps % snapshot_freq) == 0:
                self.create_snapshot()
                print('snapshot created at step {}'.format(steps))
                
            nextInstance.collapse()
            if no_constrain == False:
                if self.constrain(nextInstance) == False:
                    restarts += 1
                    if restarts > max_restarts: return False
                    
                    self.restore_snapshot()
                    print('snapshot restored at step {}'.format(steps))
                    
                    if (steps < snapshot_freq) and (len(self.snapshots) > 1):
                        self.snapshots.remove(self.snapshots[-1])
                        print('snapshot removed')
                       
                    steps = 0
                
            nextInstance = self.find_next_target_instance()
        
        print('solve success')
        return True

    def get_solution_loss(self):
        loss = 0
        acc = 0
        accTotal = 0
        
        for unit in self.units:
            for position, instance in unit.instances.items():
                if instance.val >= 0:
                    instance.rebuild_dist()
                    total = np.sum(instance.dist)
                    
                    fraction = instance.dist[instance.val] / total
                    loss -= np.nan_to_num(np.log2(fraction), 0)
                    
                    accTotal += 1
                    if np.argmax(instance.dist) == instance.val: acc += 1
        
        if accTotal > 0: acc /= accTotal
        return loss, acc
        
    def update_masks(self, cleaningThreshold=0.0001, minMaskCount=32):
        maskBitsUpdated = 0
        
        for unit in self.units:
            for link in unit.links:
                for val in range(unit.width * 64):
                    total = np.sum(link.weights[val])
                    if total < minMaskCount: continue
                    for otherVal in range(link.unitTo.width * 64):
                        fraction = (link.weights[val, otherVal] + 1) / total
                        if fraction < (cleaningThreshold / (link.unitTo.width * 64)):
                            oldMaskBit = bitset.get_bit(link.mask[val], otherVal)
                            bitset.unset_bit(link.mask[val], otherVal)
                            link.maskCount -= oldMaskBit
                            maskBitsUpdated -= oldMaskBit
                    
        return maskBitsUpdated
    
    #"""
    def update_weights(self, scale=1):
        for unit in self.units:
            for position, instance in unit.instances.items():
                if instance.val < 0: continue
                for i in range(len(instance.instanceToLinks)):
                    otherInstance = instance.instanceToLinks[i][0]
                    if otherInstance.val < 0: continue
                    instance.instanceToLinks[i][1].weights[instance.val, otherInstance.val] = np.maximum(instance.instanceToLinks[i][1].weights[instance.val, otherInstance.val] + scale, 0)
        return
    #"""
    
def load_model(epoch):
    modelPath = '{0}/model.{1}.rmx'.format(MODEL_DIR, epoch)
    return pickle.load(open(modelPath, 'rb'))
    
def save_model(model, epoch):
    model.clear_snapshots()
    model.clear_instances()
    modelPath = '{0}/model.{1}.rmx'.format(MODEL_DIR, epoch)
    pickle.dump(model, open(modelPath, 'wb'))
    return

def dump_model_masks(model, file):
    masks = np.zeros(0, dtype=np.uint64)
    for unit in model.units:
        for link in unit.links:
            for val in range(unit.width * 64):
                temp = np.zeros(len(link.mask[val]) * 64, dtype=np.uint64)
                bitset.sum_list(temp, [link.mask[val]])
                masks = np.concatenate((masks, temp), axis=None)
    (masks.astype(np.int8) * 64).tofile(file)
    return
    
def build_model():
    model = Model()
 
    #offsets0 = [(+1,)]
    #unit0 = model.add_unit_class(4)
    #for offset in offsets0:
    #    unit0.add_link(unit0, offset).init_all()
 
    
    offsets0 = [(+1,)]
    unit0 = model.add_unit_class(4)
    for offset in offsets0:
        unit0.add_link(unit0, offset).init_all()
    
    offsets0 = [(+8,)]
    offsets1 = [(+1,),(+2,),(+3,),(+4,),(+5,),(+6,),(+7,),(+8,),(+9,),(+10,),(+11,),(+12,)]
    unit1 = model.add_unit_class(4)
    for offset in offsets0:
        unit1.add_link(unit1, offset).init_all()
    for offset in offsets1:
        unit1.add_link(unit0, offset).init_all()
            
    offsets0 = [(+64,)]
    offsets2 = [(+0,),(+8,),(+16,),(+32,),(+48,),(+64,)]
    unit2 = model.add_unit_class(6)
    for offset in offsets0:
        unit2.add_link(unit2, offset).init_all()
    for offset in offsets2:
        unit2.add_link(unit1, offset).init_all()
        
        
    return model
    

def test_sample():
    model = load_model(0)
    
    #dump_model_masks(model, 'masks.raw')
    
    testData = np.zeros(4000, dtype=np.uint8)
    for i in range(len(testData)):
        model.set_instance_val(0, (i,))       
    for i in range(0, len(testData), 8):
        model.set_instance_val(1, (i,))
    for i in range(0, len(testData), 64):
        model.set_instance_val(2, (i,))
            
    model.set_instance_val(0, (0,), 32)
    
    model.relink_instances()
    print(model.solve(snapshot_freq=100))

    #model.update_weights(-9999999)
    #print('mask bits updated: {}'.format(model.update_masks()))

    for i in range(len(testData)):
        testData[i] = model.get_instance_val(0, (i,)) & 0xFF
    testData.tofile('testdata.raw')
    
    save_model(model, 0)
    
    return
    
def test_train():
    testData = np.fromfile('lgprocess.py', dtype=np.uint8)
    model = build_model()
    #model = load_model(0)
    
    for iter in range(1):
        print('beginning iteration {}'.format(iter))
        for i in range(len(testData)):
            model.set_instance_val(0, (i,), testData[i])
        for i in range(0, len(testData), 8):
            model.set_instance_val(1, (i,))
        for i in range(0, len(testData), 64):
            model.set_instance_val(2, (i,))
            
        model.relink_instances()
        model.solve(snapshot_freq=1000)
        model.update_weights(10000000)
        """
        for unit in model.units:
            for link in unit.links:
                link.weights *= (link.weights - 1) * 1000000000  
        """
        print('loss : {}'.format(model.get_solution_loss()))
        print('mask bits updated: {}'.format(model.update_masks()))
    
    save_model(model, 0)    
    
if __name__ == '__main__':

    bitset.seed_random()

    test_train()
    test_sample()
    
    

 
    
