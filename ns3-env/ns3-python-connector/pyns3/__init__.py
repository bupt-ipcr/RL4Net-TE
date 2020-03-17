from gym.envs.registration import register

register(
    id='ns3-v0',
    entry_point='pyns3.ns3env:Ns3Env',
)