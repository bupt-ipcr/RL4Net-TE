#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 1970-01-01 08:00
@edit time: 2020-03-04 13:40
@FilePath: /RL_brain/eval-drlte/simple_test.py
"""


import argparse
import random
import numpy as np
import json
from ns3gym import ns3env
from ddpg import DDPG
import numpy as np

__author__ = "Piotr Gawlowicz"
__copyright__ = "Copyright (c) 2018, Technische Universität Berlin"
__version__ = "0.1.0"
__email__ = "gawlowicz@tkn.tu-berlin.de"


def get_action():
    adjacencyMatrix = [0, 1, 1, 1, -1, 0, -1, 1, -1, -1, 0, 1, -1, -1, -1, 0]  # 使用四节点测试
    agent = DDPG(len(adjacencyMatrix), adjacencyMatrix.count(1), a_bound=1)
    agent.load()
    state_str = '''[{/src/:0,/dst/:3,/rate/:5}]'''
    state_list = json.loads(state_str.replace('/', '"'))
    state = np.zeros(16)
    for s in state_list:
        state[s['src'] * 4 + s['dst']] = s['rate']
    return agent.get_action(state), agent.get_action_noise(state, rate=1)


def get_rl_rewards():
    simArgs = {"--maxStep": 100,
               "--routingMethod": 'rl',
               "--adjacencyMatrix": '''[0,1,1,1,-1,0,-1,1,-1,-1,0,1,-1,-1,-1,0]''',
               "--trafficMatrix": '''[{/src/:0,/dst/:3,/rate/:5}]'''
               }

    print('  start a sim')
    env = ns3env.Ns3Env(port=5566, startSim=True, simSeed=0, simArgs=simArgs, simScriptName='udp-fm')
    obs = env.reset()
    ac_space = env.action_space
    cum_reward = 0
    while True:
        action = env.action_space.sample()
        print("---action: ", action)

        obs, reward, done, _ = env.step(action)
        print("reward, done: ", reward, done)
        cum_reward += reward
        if done:
            stepIdx = 0
            break
    if env:
        env.close()
    return cum_reward


def get_rl_reward():
    simArgs = {"--maxStep": 2,
               "--routingMethod": 'rl',
               "--adjacencyMatrix": '''[0,1,1,1,-1,0,-1,1,-1,-1,0,1,-1,-1,-1,0]''',
               "--trafficMatrix": '''[{/src/:0,/dst/:3,/rate/:5}]'''
               }

    print('  start a sim')
    env = ns3env.Ns3Env(port=5566, startSim=True, simSeed=0, simArgs=simArgs, simScriptName='udp-fm')
    obs = env.reset()
    ac_space = env.action_space

    action = [1, 1, 5, 1, 1]
    obs, reward, done, _ = env.step(action)
    print("reward, done: ", reward, done)
    if env:
        env.close()
        
    return reward


if __name__ == '__main__':
    print('action:', get_action())
