# CS 4491 Coursework: LoRa Networks

Other LoRa work from the same course (Kennesaw State University, Spring 2025). This is kept as class material, separate from the [gate opener final project](../README.md).

## `rl-transmission-power/`: learning LoRa transmit power with a DQN

A deep Q-network that learns per-node **transmit power** for a LoRa network, with the goal of cutting total energy use.

**`learningTP.py`** is the DQN agent (TensorFlow/Keras):

- Each episode places 100 nodes at random positions.
- The agent picks a transmit power (2–14 dBm) for every node and runs a network simulation.
- The reward is the negative of total energy consumed.
- Experiences go into a replay buffer, and the reward curve is plotted at the end.

### The simulator (not included)

The agent drives a modified copy of `loraDir.py` from **LoRaSim** (Voigt & Bor, ["Do LoRa Low-Power Wide-Area Networks Scale?"](http://dx.doi.org/10.1145/2988287.2989163), MSWiM '16). LoRaSim is third-party software, so it isn't redistributed here. To run the agent, get LoRaSim from its authors and adapt `loraDir.py` as follows:

- Wrap the simulation setup and run in a function `startsimulation(locations, txpows)`, so it can be called with given node positions and per-node transmit powers instead of from the command line.
- Return `(nrCollisions, energy, sent, nrReceived, nrProcessed, nrLost, der)` from that function.
- Update it for Python 3. The original's Python 2 `print` statements are the main blocker.

Put the adapted `loraDir.py` next to `learningTP.py`, then:

```bash
pip install -r requirements.txt
python learningTP.py
```

### Known limitations (left as submitted)

- `epsilon_decay = 0.0995` is probably meant to be `0.995`. As written, exploration collapses to the minimum after about two training steps.
- Each episode is a single step (`next_state = state`), so this behaves more like a contextual bandit than full sequential RL.
