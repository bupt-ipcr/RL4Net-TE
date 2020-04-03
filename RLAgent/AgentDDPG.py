#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-08 22:32
@edit time: 2020-04-03 11:26
@FilePath: /RLAgent/AgentDDPG.py
"""


import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
from wjwgym.agents import DDPGBase
from wjwgym.models import SimpleCriticNet
import os
CUDA = torch.cuda.is_available()


class OUProcess(object):
    """Ornstein-Uhlenbeck process"""

    def __init__(self, x_size, mu=0, theta=0.15, sigma=0.3):
        self.x = np.ones(x_size) * mu
        self.x_size = x_size
        self.mu = mu
        self.theta = theta
        self.sigma = sigma

    def noise(self):
        dx = self.theta * (self.mu - self.x) + self.sigma * np.random.randn(self.x_size)
        self.x = self.x + dx
        return self.x


class DDPG(DDPGBase):
    def _build_net(self):
        """
        actor使用一个两层的网络，softmax输出激活
        critic使用最简单的一层网络
        """
        n_states, n_actions = self.n_states, self.n_actions
        print('bound: ', self.bound)
        if CUDA:
            print(f'use pytorch with gpu')
        else:
            print(f'use pytorch with cpu')
        self.actor_eval = ActorNetwork(n_states, n_actions, a_bound=self.bound)
        self.actor_target = ActorNetwork(n_states, n_actions, a_bound=self.bound)
        self.critic_eval = SimpleCriticNet(n_states, n_actions, n_neurons=64)
        self.critic_target = SimpleCriticNet(n_states, n_actions, n_neurons=64)

    def _build_noise(self):
        self.noise = OUProcess(self.n_actions, sigma=0.1)

    def get_action_noise(self, state, rate=1):
        action = self.get_action(state)
        noise_rate = max(self.bound * rate, 1)
        action_noise = self.noise.noise() * noise_rate  # OU 噪声也要有倍率
        action = np.clip(action + action_noise, 0.01, None)
        # 先考虑不限制范围，尤其不限制下限为1
        return action

    def learn_batch(self):
        if 'learn_step' not in self.__dict__:
            self.learn_step = 0
        c_loss, a_loss = self.learn()
        if c_loss is not None:
            self.summary_writer.add_scalar('c_loss', c_loss, self.learn_step)
            self.summary_writer.add_scalar('a_loss', a_loss, self.learn_step)
            self.learn_step += 1

    # TODO 将方法固定到基类
    def save(self, episode):
        state = {
            'actor_eval_net': self.actor_eval.state_dict(),
            'actor_target_net': self.actor_target.state_dict(),
            'critic_eval_net': self.critic_eval.state_dict(),
            'critic_target_net': self.critic_target.state_dict(),
            'episode': episode,
            'learn_step': self.learn_step
        }
        torch.save(state, './drlte.pth')

    # TODO 将方法固定到基类
    def load(self):
        print('\033[1;31;40m{}\033[0m'.format('加载模型参数...'))
        if not os.path.exists('drlte.pth'):
            print('\033[1;31;40m{}\033[0m'.format('没找到保存文件'))
            return 0
        saved_state = torch.load("./drlte.pth")
        self.actor_eval.load_state_dict(saved_state['actor_eval_net'])
        self.actor_target.load_state_dict(saved_state['actor_target_net'])
        self.critic_eval.load_state_dict(saved_state['critic_eval_net'])
        self.critic_target.load_state_dict(saved_state['critic_target_net'])
        self.learn_step = saved_state['learn_step']
        return saved_state['episode'] + 1


class ActorNetwork(nn.Module):
    def __init__(self, n_states, n_actions, n1_neurons=64, n2_neurons=32, a_bound=None):
        """
        定义隐藏层和输出层参数
        @param n_obs: number of observations
        @param n_actions: number of actions
        @param n1_neurons: 第一个隐藏层神经元数目
        @param n2_neurons: 第二个隐藏层神经元数目
        @param a_bound: action的倍率
        """
        super(ActorNetwork, self).__init__()
        self.bound = a_bound if a_bound else n_actions * 2
        self.fc1 = nn.Linear(n_states, n1_neurons)
        self.fc1.weight.data.normal_(0, 0.1)
        self.fc2 = nn.Linear(n1_neurons, n2_neurons)
        self.fc2.weight.data.normal_(0, 0.1)
        self.out = nn.Linear(n2_neurons, n_actions)
        self.out.weight.data.normal_(0, 0.1)
        if CUDA:
            self.bound = torch.FloatTensor([self.bound]).cuda()
        else:
            self.bound = torch.FloatTensor([self.bound])

    def forward(self, x):
        """
        定义网络结构: 第一层网络->ReLU激活->第二层网络->ReLU激活->输出层->softmax->输出
        使用softmax是因为输出是metric，将其限制为非负
        """
        x = x.cuda() if CUDA else x
        x = self.fc1(x)
        x = F.relu(x)
        x = self.fc2(x)
        x = F.relu(x)
        x = self.out(x)
        action_value = F.softmax(x, dim=1)
        action_value = action_value * self.bound
        return action_value
