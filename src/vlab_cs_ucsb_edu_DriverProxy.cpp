/*
 * vlab_cs_ucsb_edu_DriverProxy.cpp
 *
 *  Created on: Aug 26, 2015
 *      Author: baki
 */

#include <map>
#include <string>
#include <iostream>

#include "vlab_cs_ucsb_edu_DriverProxy.h"
#include "Driver.h"


jfieldID getHandleField(JNIEnv *env, jobject obj)
{
    jclass c = env->GetObjectClass(obj);
    // J is the type signature for long:
    return env->GetFieldID(c, "driverPointer", "J");
}

template <typename T>
T *getHandle(JNIEnv *env, jobject obj)
{
    jlong handle = env->GetLongField(obj, getHandleField(env, obj));
    return reinterpret_cast<T *>(handle);
}

template <typename T>
void setHandle(JNIEnv *env, jobject obj, T *t)
{
    jlong handle = reinterpret_cast<jlong>(t);
    env->SetLongField(obj, getHandleField(env, obj), handle);
}

jobject newBigInteger(JNIEnv *env, jstring value) {
  jclass big_integer_class = env->FindClass("java/math/BigInteger");
  jmethodID big_integer_ctor = env->GetMethodID(big_integer_class, "<init>", "(Ljava/lang/String;)V");
  jobject big_integer = env->NewObject(big_integer_class, big_integer_ctor, value);
  return big_integer;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    initABC
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_initABC (JNIEnv *env, jobject obj, jint log_level) {

  Vlab::Driver *abc_driver = new Vlab::Driver();
  abc_driver->InitializeLogger((int)log_level);
  setHandle(env, obj, abc_driver);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    setOption
 * Signature: (IZ)V
 */
/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    setOption
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_setOption__I
  (JNIEnv *env, jobject obj, jint option) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  abc_driver->set_option(static_cast<Vlab::Option::Name>(option));
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    setOption
 * Signature: (ILjava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_setOption__ILjava_lang_String_2
  (JNIEnv *env, jobject obj, jint option, jstring value) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  const char* value_arr = env->GetStringUTFChars(value, JNI_FALSE);
  std::string value_str {value_arr};
  abc_driver->set_option(static_cast<Vlab::Option::Name>(option), value_str);
  env->ReleaseStringUTFChars(value, value_arr);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    isSatisfiable
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_isSatisfiable
  (JNIEnv *env, jobject obj, jstring constraint) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  std::istringstream input_constraint;
  const char* constraint_str = env->GetStringUTFChars(constraint, JNI_FALSE);
  input_constraint.str(constraint_str);
  abc_driver->reset();
  abc_driver->Parse(&input_constraint);
  env->ReleaseStringUTFChars(constraint, constraint_str);
  abc_driver->InitializeSolver();
  abc_driver->Solve();
  bool result = abc_driver->is_sat();
  return (jboolean)result;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    countVariable
 * Signature: (Ljava/lang/String;J)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_countVariable
  (JNIEnv *env, jobject obj, jstring var_name, jlong bound) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  const char* var_name_arr = env->GetStringUTFChars(var_name, JNI_FALSE);
  std::string var_name_str {var_name_arr};
  auto result = abc_driver->CountVariable(var_name_str, bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  env->ReleaseStringUTFChars(var_name, var_name_arr);
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    countInts
 * Signature: (J)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_countInts
  (JNIEnv *env, jobject obj, jlong bound) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  auto result = abc_driver->CountInts(bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    countStrs
 * Signature: (J)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_countStrs
  (JNIEnv *env, jobject obj, jlong bound) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  auto result = abc_driver->CountStrs(bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    count
 * Signature: (JJ)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_count__JJ
  (JNIEnv *env, jobject obj, jlong int_bound, jlong str_bound) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  auto result = abc_driver->Count(int_bound, str_bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    getModelCounterForVariable
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_getModelCounterForVariable
  (JNIEnv *env, jobject obj, jstring var_name) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  const char* var_name_arr = env->GetStringUTFChars(var_name, JNI_FALSE);
  std::string var_name_str {var_name_arr};
  auto mc = abc_driver->GetModelCounterForVariable(var_name_str);
  std::stringstream os;
  {
    cereal::BinaryOutputArchive ar(os);
    mc.save(ar);
  }
  jstring result_string = env->NewStringUTF(os.str().c_str());
  env->ReleaseStringUTFChars(var_name, var_name_arr);
  return result_string;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    getModelCounterForInts
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_getModelCounterForInts
  (JNIEnv *env, jobject obj) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  auto mc = abc_driver->GetModelCounterForInts();
  std::stringstream os;
  {
    cereal::BinaryOutputArchive ar(os);
    mc.save(ar);
  }
  jstring result_string = env->NewStringUTF(os.str().c_str());
  return result_string;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    getModelCounterForStrs
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_getModelCounterForStrs
  (JNIEnv *env, jobject obj) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  auto mc = abc_driver->GetModelCounterForStrs();
  std::stringstream os;
  {
    cereal::BinaryOutputArchive ar(os);
    mc.save(ar);
  }
  jstring result_string = env->NewStringUTF(os.str().c_str());
  return result_string;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    getModelCounter
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_getModelCounter
  (JNIEnv *env, jobject obj) {

  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  auto mc = abc_driver->GetModelCounter();
  std::stringstream os;
  {
    cereal::BinaryOutputArchive ar(os);
    mc.save(ar);
  }
  jstring result_string = env->NewStringUTF(os.str().c_str());
  return result_string;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    countVariable
 * Signature: (Ljava/lang/String;JLjava/lang/String;)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_countVariable__Ljava_lang_String_2JLjava_lang_String_2
  (JNIEnv *env, jobject obj, jstring var_name, jlong bound, jstring bin_model_counter) {

  const char* bin_model_counter_arr = env->GetStringUTFChars(bin_model_counter, JNI_FALSE);
  std::string bin_model_counter_str {bin_model_counter_arr};
  std::stringstream is (bin_model_counter_str);
  Vlab::Solver::ModelCounter mc;
  {
    cereal::BinaryInputArchive ar(is);
    mc.load(ar);
  }
  env->ReleaseStringUTFChars(bin_model_counter, bin_model_counter_arr);
  auto result = mc.Count(bound, bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    countInts
 * Signature: (JLjava/lang/String;)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_countInts__JLjava_lang_String_2
  (JNIEnv *env, jobject obj, jlong bound, jstring bin_model_counter) {

  const char* bin_model_counter_arr = env->GetStringUTFChars(bin_model_counter, JNI_FALSE);
  std::string bin_model_counter_str {bin_model_counter_arr};
  std::stringstream is (bin_model_counter_str);
  Vlab::Solver::ModelCounter mc;
  {
    cereal::BinaryInputArchive ar(is);
    mc.load(ar);
  }
  env->ReleaseStringUTFChars(bin_model_counter, bin_model_counter_arr);
  auto result = mc.CountInts(bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    countStrs
 * Signature: (JLjava/lang/String;)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_countStrs__JLjava_lang_String_2
  (JNIEnv *env, jobject obj, jlong bound, jstring bin_model_counter) {

  const char* bin_model_counter_arr = env->GetStringUTFChars(bin_model_counter, JNI_FALSE);
  std::string bin_model_counter_str {bin_model_counter_arr};
  std::stringstream is (bin_model_counter_str);
  Vlab::Solver::ModelCounter mc;
  {
    cereal::BinaryInputArchive ar(is);
    mc.load(ar);
  }
  env->ReleaseStringUTFChars(bin_model_counter, bin_model_counter_arr);
  auto result = mc.CountStrs(bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    count
 * Signature: (JJLjava/lang/String;)Ljava/math/BigInteger;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_count__JJLjava_lang_String_2
  (JNIEnv *env, jobject obj, jlong int_bound, jlong str_bound, jstring bin_model_counter) {

  const char* bin_model_counter_arr = env->GetStringUTFChars(bin_model_counter, JNI_FALSE);
  std::string bin_model_counter_str {bin_model_counter_arr};
  std::stringstream is (bin_model_counter_str);
  Vlab::Solver::ModelCounter mc;
  {
    cereal::BinaryInputArchive ar(is);
    mc.load(ar);
  }
  env->ReleaseStringUTFChars(bin_model_counter, bin_model_counter_arr);
  auto result = mc.Count(int_bound, str_bound);
  std::stringstream ss;
  ss << result;
  jstring result_string = env->NewStringUTF(ss.str().c_str());
  return newBigInteger(env, result_string);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    printResultAutomaton
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_printResultAutomaton__ (JNIEnv *env, jobject obj) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  if (abc_driver->is_sat()) {
    int index = 0;
    for(auto& variable_entry : abc_driver->getSatisfyingVariables()) {
      abc_driver->printResult(variable_entry.second, std::cout);
    }
    std::cout.flush();
  }
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    printResultAutomaton
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_printResultAutomaton__Ljava_lang_String_2 (JNIEnv *env, jobject obj, jstring filePath) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  const char* file_path_str = env->GetStringUTFChars(filePath, JNI_FALSE);
  std::string file_path {file_path_str};

  if (abc_driver->is_sat()) {
    int index = 0;
    for(auto& variable_entry : abc_driver->getSatisfyingVariables()) {
      std::stringstream ss;
      ss << file_path << "_" << index << ".dot";
      abc_driver->inspectResult(variable_entry.second, ss.str());
    }
    std::cout.flush();
  }
  env->ReleaseStringUTFChars(filePath, file_path_str);
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    getSatisfyingExamples
 * Signature: ()Ljava/util/Map;
 */
JNIEXPORT jobject JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_getSatisfyingExamples (JNIEnv *env, jobject obj) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  jclass hashMapClass = env->FindClass("java/util/HashMap");
  jmethodID hashMapCtor = env->GetMethodID(hashMapClass, "<init>", "()V");
  jobject map = env->NewObject(hashMapClass, hashMapCtor);

  std::map<std::string, std::string> results = abc_driver->getSatisfyingExamples();

  jmethodID hasMapPut = env->GetMethodID(hashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

  for (auto var_entry : results) {
    jstring var_name = env->NewStringUTF(var_entry.first.c_str());
    jstring var_value = env->NewStringUTF(var_entry.second.c_str());
    env->CallObjectMethod(map, hasMapPut, var_name, var_value);
  }

  return map;
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_reset (JNIEnv *env, jobject obj) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  abc_driver->reset();
}

/*
 * Class:     vlab_cs_ucsb_edu_DriverProxy
 * Method:    dispose
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_vlab_cs_ucsb_edu_DriverProxy_dispose (JNIEnv *env, jobject obj) {
  Vlab::Driver *abc_driver = getHandle<Vlab::Driver>(env, obj);
  Vlab::Driver *tmp = abc_driver;
  abc_driver = nullptr;
  setHandle(env, obj, abc_driver);
  delete tmp;
}
