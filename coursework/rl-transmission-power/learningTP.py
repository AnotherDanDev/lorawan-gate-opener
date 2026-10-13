import loraDir as simulator
import random
import numpy as np
import tensorflow as tf
from collections import deque
import random
import matplotlib.pyplot as plt

def learn():
    nrNodes =100   #Number of user devices
    n_episodes = 400
    batch_size = 32
    state_size = nrNodes * 2 # state is list of locations of user devices stored in form of (x,y). 
    action_size = nrNodes  #Action is a list of transmission powers for each user devices
    agent = DQNAgent(state_size, action_size) #Create the DQN model

    for e in range(n_episodes):
        locations =[]
        for i in range(0,nrNodes):  #We consider dynamic users, thus initializing the locations of users for each iteration.
            x = random.random()
            y = random.random()
            locations.append([x,y])

        #Initializing environment which contains state, action, reward
        
        
        # state is locations stored as list of values. It is stored as [x1 y1 x2 y2 x3 y3....]
        state = np.reshape(locations, [1, state_size])       
        
        # Select action by predicting the action using our dQN model 'agent'. This is finding of transmission powers for each devices
        action = agent.act(state)
        
        # Once we get the locations and transmission powers (Action) for all users, we input it to the simulator and find how the environemnt
        #performs. At the end of simulation, we recieve nrCollisions, energy, sent, nrReceived, nrProcessed, nrLost, der as output which are 
        #performance metrics for the given simulation
        nrCollisions, energy, sent, nrReceived, nrProcessed, nrLost, der =simulator.startsimulation(locations,action)
        
        #Since we are interested in optimizing the energy consumption, reward is set as energy. But we want to minimize the enrgy consumption
        #But DQN maximizes the reward. Thus we set reward as negative of energy. Thus when DQN maximizes the reward, it minimizes the energy.
        reward = -1 * energy
        
        
        next_state = state
        
        # Store experience in replay memory
        agent.remember(state, action, reward, next_state )
        
        nrReceived = sent - nrCollisions
        print(nrCollisions, energy, sent, nrReceived, der)
        
        #store the reward  of each episode for plotting the graph 
        agent.rewards.append(reward)
         
        # Train the agent with the experience
        if len(agent.memory) > batch_size:
            agent.replay(batch_size)        
    
    plt.plot(agent.get_rewards())
    plt.xlabel('Episode')
    plt.ylabel('Total Reward')
    plt.title('Reward over episodes')
    plt.show()
    
    
class DQNAgent:
    def __init__(self, state_size, action_size):
        self.state_size = state_size
        self.action_size = action_size
        self.memory = deque(maxlen=2000)
        
        self.epsilon = 1.0  # exploration rate
        self.epsilon_min = 0.01
        self.epsilon_decay = 0.0995
        self.learning_rate = 0.01
        self.gamma=0.95
        self.model = self._build_model()
        self.rewards = []
        
        #return the rewards stored in agent for each episodes
    def get_rewards(self):
        return self.rewards
    
    def _build_model(self): #builidng entire neural network to train based in input
        model = tf.keras.models.Sequential() 
        model.add(tf.keras.layers.Dense(24, input_dim=self.state_size, activation='relu')) #24 neurons in input layer
        model.add(tf.keras.layers.Dense(24, activation='relu')) # 24 in hidden layer
        model.add(tf.keras.layers.Dense(self.action_size, activation='sigmoid')) # output layer which gives all transmission powers for all devices
        model.compile(loss='mse', optimizer=tf.keras.optimizers.Adam(learning_rate=self.learning_rate)) #loss function used in mean squared error
        return model
    
    
  
    #store the previous events to memory
    def remember(self, state, action, reward, next_state):
        self.memory.append((state, action, reward, next_state))

    # It predicts the action for given state.
    def act(self, state):
        # This step adds randmization in prediction. It avoids overfitting in neural network
        if np.random.rand() <= self.epsilon:
            return [random.randrange(2, 14, 1) for i in range(self.action_size)] #range of transmission powers is from 2db ~ 14db
        # model predicts the action values. Since the output layer uses sigmoid function, output is between 0~1    
        act_values = self.model.predict(state)
        
        #converting range of output from 0~1 to 2~14
        return [int(12*i+2) for i in act_values[0]]

    #After storing sufficient amount of data (batch_size), model uses this data to train itself. 
    def replay(self, batch_size):
        
        minibatch = random.sample(self.memory, batch_size) # take batchsize amount of data as minibatch that is training set
        for state, action, reward, next_state in minibatch:
            target = (reward + self.gamma * self.model.predict(next_state)) # for each row, predict what should be transmission powers
            self.model.fit(state, target, epochs=1, verbose=False) # based on error, fit the training model
        if self.epsilon > self.epsilon_min:  # Epsilon decay is a method to balance “exploitation and exploration”. At the start, high epsilon is used to explore the environment. At the end, low epsilon is used to exploit the environment.
            self.epsilon *= self.epsilon_decay
            
            
learn()


