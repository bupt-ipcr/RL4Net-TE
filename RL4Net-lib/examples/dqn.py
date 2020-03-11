#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-07 20:17
@edit time: 2020-01-15 17:00
@file: /examples/dqn.py
"""
import torch
import numpy as np
import wjwgym
from functools import reduce
import gym
from wjwgym.agents import DQNBase
from wjwgym.models import SimpleDQNNet
CUDA = torch.cuda.is_available()
from torch.utils.tensorboard import SummaryWriter


class DQN(DQNBase):
    """
    基于DQNBase创建的DQN类，通过附带的简单神经网络创建了 eval dqn network 和 target dqn network
    """
    def _build_net(self):
        self.eval_net = SimpleDQNNet(self.n_states, self.n_actions)
        self.target_net = SimpleDQNNet(self.n_states, self.n_actions)


def rl_loop():
    summary_writer = SummaryWriter()
    MAX_EPISODES = 100
    env = gym.make('Maze-v0')
    n_states = reduce(np.multiply, env.observation_space.shape)
    n_actions = env.action_space.n
    agent = DQN(n_states, n_actions)
    # train
    for ep in range(MAX_EPISODES):
        cur_state = env.reset()
        cur_state = cur_state.reshape((n_states))
        done = False
        while not done:
            action = agent.get_action(cur_state)
            next_state, reward, done, info = env.step(action)
            next_state = next_state.reshape((n_states))
            agent.add_step(cur_state, action, reward, done, next_state)
            loss = agent.learn()
            if loss:
                summary_writer.add_scalar('loss', loss, agent.eval_step)
            cur_state = next_state
        print('ep: ', ep, ' steps: ', info, ' final reward: ', reward)
        summary_writer.add_scalar('reward', reward, ep)
    print('done')


if __name__ == '__main__':
    rl_loop()
