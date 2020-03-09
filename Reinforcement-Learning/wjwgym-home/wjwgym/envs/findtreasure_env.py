#!/usr/bin/env python
# coding=utf-8
"""
@create time: 2019-11-22 10:21
@author: Jiawei Wu
@edit time: 2019-11-25 11:28
@file: /find_treasure.py
"""
import numpy as np
from gym.spaces import Discrete, Box
import gym

class FindTreasureEnv(gym.Env):
    def __init__(self, length=7, treasures=[6]):
        self.player_pos = 0
        self.treasures = treasures
        obs_states = [-1, 0, 1]
        self.player, self.road, self.treasure = obs_states

        self.init_obs = np.array([self.road] * length)
        for t in treasures:
            self.init_obs[t] = self.treasure
        self.observation_space = Box(shape=(length, ), low=min(obs_states), high=max(obs_states))
        self.action_space = Discrete(2)

        head_left, head_right = 0, 1
        ac_states = [head_left, head_right]

        self.step_count = 0
    
    def reset(self):
        self.player_pos = 0
        obs = self.init_obs.copy()
        obs[self.player_pos] = self.player
        self.step_count = 0

        return obs

    def get_obs(self):
        obs = self.init_obs.copy()
        obs[self.player_pos] = self.player
        return obs
        
    def get_reward(self):
        move, win = 0, 1
        if any((self.player_pos == t_pos for t_pos in self.treasures)):
            return win
        else:
            return move

    def get_done(self):
        if any((self.player_pos == t_pos for t_pos in self.treasures)):
            return True
        else:
            return False
            
    def get_info(self):
        self.step_count += 1
        return self.step_count
        
    def step(self, action):
        if action == 0:
            delta = -1
        else:
            delta = 1
        next_pos = self.player_pos + delta
        if 0 <= next_pos < self.observation_space.shape[0]:
            self.player_pos = next_pos
 
        obs = self.get_obs()
        reward = self.get_reward()
        done = self.get_done()
        info = self.get_info()
        return obs, reward, done, info

    def render(self):
        ascii_map = {
            -1: '*',
            0: '-',
            1: 'o'
        }
        print(f'{"".join([ascii_map[d] for d in self.get_obs()])}\r', end='')




    