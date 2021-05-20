# ModularTest

Welcome to my ModularTest Repository. This is where I test things for the custom modular system of my vst3 plugins.

update 2021_05_17b:

thread-safety & realtime-safety probably improved with spinLocks in ThreadSafeObject.

update 2021_05_17:
this thing became pretty huge already. it has an almost fully working and very flexible modular system with an infinite amount of modulators theoretically (not practically, because amounts of parameters are limited in vst plugins). it already has 3 different types of modulators in it, makros, envelope follower and an lfo with wavetables that can be set flexibly as well and it's all processing in stereo, including parameter smoothing. every modulator can have an infinite amount of modulation destinations (also theoretically ofc).

next goals are adding functionality for bidirectional modulation destination (atm it just goes into the direction you drag the thing), and i want to improve my thread-safety implementation. btw if you came here because of that, maybe because you watch my youtube channel or something, sorry, but there are still issues that have been found. yes, it's already pretty good, but it's not yet as good as i said in my videos.
