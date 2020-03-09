#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-11-25 11:08
@edit time: 2020-01-15 16:20
@file: /wjwgym-home/setup.py
"""


from setuptools import setup, find_packages
import sys


def readme():
    with open('README.md') as f:
        return f.read()


setup(
    name='wjwgym',
    version='0.2.0',
    packages=find_packages(),
    scripts=[],
    url='',
    license='MIT',
    author='Jiawei Wu',
    author_email='13260322877@163.com',
    description='Reinforcement Learning Tools',
    long_description='Agent(Q-Learning, DQN, DDPG..) base classes; Simple pytorch models; simple openai gym envs; and examples.',
    keywords=['Reinforcement Learning', 'OpenAI gym', 'Tools'],
    install_requires=['numpy', 'gym', 'torch', 'pandas'],
    extras_require={},
)
