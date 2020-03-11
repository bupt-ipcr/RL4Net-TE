#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-11-25 10:49
@edit time: 2020-01-15 16:14
@file: /wjwgym/wjwgym/__init__.py
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