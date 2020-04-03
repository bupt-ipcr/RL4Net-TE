#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2020-03-30 21:33
@edit time: 2020-03-31 22:36
@desc: 验证rl4net是否正常工作
"""

import numpy as np
import os
import pyns3
from pyns3 import ns3env
import pytest
from wjwgym.agents import DDPGBase
from wjwgym.models import SimpleCriticNet, SimpleActorNet


class DDPG(DDPGBase):
    """用于构建简单agent"""

    def _build_net(self):
        """
        actor使用一个两层的网络，softmax输出激活
        critic使用最简单的一层网络
        """
        n_states, n_actions = self.n_states, self.n_actions
        print('bound: ', self.bound)
        self.actor_eval = SimpleActorNet(n_states, n_actions, a_bound=self.bound)
        self.actor_target = SimpleActorNet(n_states, n_actions, a_bound=self.bound)
        self.critic_eval = SimpleCriticNet(n_states, n_actions, n_neurons=64)
        self.critic_target = SimpleCriticNet(n_states, n_actions, n_neurons=64)

    def _build_summary_writer(self):
        self.summary_writer = None

    def _build_noise(self):
        pass

    def get_action_noise(self, state, rate=1):
        action = self.get_action(state)
        noise_rate = max(self.bound * rate, 1)
        action_noise = (np.random.random(self.n_actions) - 0.5) * rate * self.bound * noise_rate  # 噪声也要有倍率
        action = np.clip(action + action_noise, 0.01, None)
        return action

    def learn_batch(self):
        if 'learn_step' not in self.__dict__:
            self.learn_step = 0
        c_loss, a_loss = self.learn()
        if c_loss is not None:
            self.learn_step += 1


class TestPyns3():
    """验证pyns3是否正常工作"""

    @classmethod
    def setup_class(cls):
        """所有测试开始前初始化env"""
        simArgs = {"--maxStep": 2}
        env = ns3env.Ns3Env(port=5555, stepTime=0.5, startSim=True, simSeed=0, simArgs=simArgs, simScriptName='udp-fm')
        cls.env = env
        cls.ob_shape = env.observation_space.shape

    @classmethod
    def teardown_class(cls):
        """确保环境被关闭"""
        env = cls.env
        if env:
            env.close()

    def test_space(self):
        """检测环境能否正常返回状态空间和动作空间的大小"""
        env = self.env
        ob_shape = self.ob_shape
        ob_space = env.observation_space
        ac_space = env.action_space
        # 检验ob_space是不是uint64型的空间
        assert ob_space.dtype == 'uint64'
        # 检验ac_space是不是float64型的空间
        assert ac_space.dtype == 'float64'

    def test_reset(self):
        """检测环境能否正常reset并返回state"""
        env = self.env
        ob_shape = self.ob_shape
        # 重置env
        obs = env.reset()
        np_obs = np.array(obs)

        # 检测obs的大小是否与ob_shape一致
        assert np_obs.shape == ob_shape

    def test_step(self):
        """测试能否正常运行env.step()"""
        env = self.env
        ob_shape = self.ob_shape
        # 产生随机动作
        action = env.action_space.sample()

        # 尝试执行
        next_state, reward, done, _ = env.step(action)

        # 检验返回值的类型
        assert np.array(next_state).shape == ob_shape   # 检验s_的大小
        assert isinstance(reward, float)    # 检验reward的类型
        assert isinstance(done, bool)  # 检验done的类型

    def test_close(self):
        """检测能否正确关闭env"""
        env = self.env
        assert env  # 先检验env是否存在
        env.close()  # 尝试关闭env

        # 检测是否关闭
        assert env.ns3ZmqBridge == None


class TestRL4Netlib():
    """验证RL4Netlib是否正常工作"""
    @classmethod
    def setup_class(cls):
        """所有测试开始前初始化agent"""
        cls.n_states, cls.n_actions = 4, 16
        cls.agent = DDPG(cls.n_states, cls.n_actions, a_bound=1, MAX_MEM=100, MIN_MEM=64)

    def test_get_action(self):
        """测试agent能否正确获取action"""
        agent = self.agent
        # 获取随机state
        random_state = np.random.random(self.n_states)
        # 尝试获取动作
        action = agent.get_action(random_state)[0]
        assert action.shape == (self.n_actions, )
        # 尝试获取带噪声的动作
        action_noise = agent.get_action_noise(random_state)[0]
        assert action_noise.shape == (self.n_actions, )
        
    def test_learn_batch(self):
        """测试训练是否正常"""
        agent = self.agent
        # 获取随机step信息
        random_state = np.random.random(self.n_states)
        random_reward = np.random.rand()
        random_done = np.random.choice([True, False])
        random_next_state = np.random.random(self.n_states)
        
        # 从agent获取动作（注意不加噪声）
        action = agent.get_action(random_state)[0]
        
        # 添加这一个step
        agent.add_step(random_state, action, random_reward, random_done, random_next_state)
        # 尝试学习
        agent.learn_batch()
        # 在没有足够数量的输出之前应该没有学习
        assert agent.learn_step == 0
        assert (agent.get_action(random_state)[0] == action).all()
        
        # 循环添加step直到超过阈值
        for _ in range(64):
            agent.add_step(random_state, action, random_reward, random_done, random_next_state)
        agent.learn_batch()
        # 此时应该学习了
        assert agent.learn_step >= 0
        assert (agent.get_action(random_state) != action).any()
        