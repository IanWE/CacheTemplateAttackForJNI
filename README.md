# CacheTemplateAttackForJNI



Use CacheTemplateAttack to inspect what functions are being used by other apps or the system.




1. The lib[libflush.so](https://github.com/IanWE/CacheTemplateAttackForJNI/tree/master/ccattack/app/src/main/cpp/libflush) need to be compiled first.
2. `libtxt/` contains codes to download libraries from the android system; please run `python get_so.py` and `python get_funcs.py` to collect address files.

3. Then you can install the app, and the switch `schedule` is to start scan, and button `hit` is to access the target function.
