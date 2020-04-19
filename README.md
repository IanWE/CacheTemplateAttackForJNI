# CacheTemplateAttackForJNI
modified edition for CTA


ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_PROJECT_PATH=`pwd` 
cp -r libs/* /projects/JNI/app/src/main/JniLibs/ 
adb logcat >test 
cat test|grep ARMAGEDDON >test1 

error1: ARMv8从子线程访问内存，出现记时错误。 LDR{条件} 目的寄存器，<存储器地址>
  `LDR   R0，[R1]                  ；将存储器地址为R1的字数据读入寄存器R0。
  inline void
  arm_v8_access_memory(void* pointer)
  {
    volatile uint32_t value;
    asm volatile ("LDR %0, [%1]\n\t"
        : "=r" (value)
        : "r" (pointer)
        );
  }`
  
  /home/finder/Android/Sdk/ndk/21.0.6113669/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64/bin/aarch64-linux-android-objdump -d libc.so

