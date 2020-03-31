#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 1970-01-01 08:00
@edit time: 2020-03-31 22:14
@desc: 简单的demo
"""


import argparse
import random
import numpy as np
import json
from pyns3 import ns3env
import numpy as np

__author__ = "Jiawei Wu"
__copyright__ = "Copyright (c) 2020, Beijing University of Posts and Telecommunications."
__version__ = "0.1.0"
__email__ = "cloudsae@bupt.edu.cn"



simArgs = {"--maxStep": 10,
            "--routingMethod": 'rl',
            "--adjacencyMatrix": '''[0,1,1,1,-1,0,-1,1,-1,-1,0,1,-1,-1,-1,0]''',
            "--trafficMatrix": '''[{/src/:0,/dst/:3,/rate/:4.736}]'''
            }

print('  start a sim')

env = ns3env.Ns3Env(port=5555, stepTime=0.5, startSim=True, simSeed=0, simArgs=simArgs, simScriptName='udp-fm')
ob_space = env.observation_space
ac_space = env.action_space
print("Observation space: ", ob_space,  ob_space.dtype)
print("Action space: ", ac_space, ac_space.dtype)
obs = env.reset()
print("---obs: ", obs)

stepIdx = 0
try:
    while True:
        mid_weight = stepIdx * 0.1
        lr_weight = (1 - mid_weight) / 2
        action = [lr_weight, lr_weight, mid_weight, 1, 1]
        print("---action: ", action)

        print("Step: ", stepIdx)
        obs, reward, done, _ = env.step(action)
        print("---pkgs, reward, done: ", sum(obs[:4]), reward, done)
        stepIdx += 1
        if done:
            stepIdx = 0
            break
finally:
    if env:
        env.close()



