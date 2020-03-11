#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-06 23:15
@edit time: 2020-01-15 16:15
@desc: envs的init文件
"""


from gym.envs.registration import register

register(
    id='FindTreasure-v0',
    entry_point='wjwgym.envs.findtreasure_env:FindTreasureEnv',
)
register(
    id='Maze-v0',
    entry_point='wjwgym.envs.maze_env:MazeEnv',
)
