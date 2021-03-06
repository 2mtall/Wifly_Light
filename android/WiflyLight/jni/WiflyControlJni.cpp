/**
		Copyright (C) 2012, 2013 Nils Weiss, Patrick Bruenn.

    This file is part of Wifly_Light.

    Wifly_Light is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Wifly_Light is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Wifly_Light.  If not, see <http://www.gnu.org/licenses/>. */

#include "BroadcastReceiver.h"
#include "WiflyControl.h"
#include <unistd.h>
#include <jni.h>

static WiflyControl* g_pControl = NULL;

void ThrowJniException(JNIEnv* env, const FatalError& e) {
		jclass javaException = env->FindClass(e.GetJavaClassType());
		env->ThrowNew(javaException, e.what());
}

#define TRY_CATCH_RETURN_BOOL(EXPRESSION) \
	try { \
		(EXPRESSION); \
		return true; \
	} catch(FatalError& e) { \
		ThrowJniException(env, e); \
	}

extern "C" {
jlong Java_biz_bruenn_WiflyLight_RemoteCollector_createBroadcastReceiver(JNIEnv* env, jobject ref)
{
	return (jlong) new BroadcastReceiver(BroadcastReceiver::BROADCAST_PORT);
}

void Java_biz_bruenn_WiflyLight_RemoteCollector_releaseBroadcastReceiver(JNIEnv* env, jobject ref, jlong pNative)
{
	delete (BroadcastReceiver*)pNative;
}

jlong Java_biz_bruenn_WiflyLight_RemoteCollector_getNextRemote(JNIEnv* env, jobject ref, jlong pNative, jlong timeoutNanos)
{
	timeval timeout;
	timeout.tv_sec = timeoutNanos / 1000000000L;
	timeout.tv_usec = (timeoutNanos % 1000000000L) / 1000L;
	return ((BroadcastReceiver*)pNative)->GetNextRemote(&timeout).AsUint64();
}

jlong Java_biz_bruenn_WiflyLight_WiflyControl_create(JNIEnv* env, jobject ref, jint ipv4Addr, jshort port)
{
	// TODO make this threadsafe
	if(NULL == g_pControl) {
		try {
			g_pControl = new WiflyControl(ipv4Addr, port);
			return reinterpret_cast<jlong>(g_pControl);
		} catch (FatalError& e) {
			g_pControl = NULL;
			ThrowJniException(env, e);
		}
	}
	return NULL;
}

jstring Java_biz_bruenn_WiflyLight_WiflyControl_ConfGetSsid(JNIEnv* env, jobject ref, jlong pNative)
{
	std::string mySsid = reinterpret_cast<WiflyControl*>(pNative)->ConfGetSsid();
	return env->NewStringUTF(mySsid.data());
}

jboolean Java_biz_bruenn_WiflyLight_WiflyControl_ConfSetWlan(JNIEnv* env, jobject ref, jlong pNative, jstring passphrase, jstring ssid)
{
	const char* myPassphrase = env->GetStringUTFChars(passphrase, 0);
	const char* mySsid = env->GetStringUTFChars(ssid, 0);
	jboolean result = reinterpret_cast<WiflyControl*>(pNative)->ConfSetWlan(myPassphrase, mySsid);
	env->ReleaseStringUTFChars(passphrase, myPassphrase);
	env->ReleaseStringUTFChars(ssid, mySsid);
	return result;
}

jboolean Java_biz_bruenn_WiflyLight_WiflyControl_FwClearScript(JNIEnv* env, jobject ref, jlong pNative)
{
	TRY_CATCH_RETURN_BOOL(reinterpret_cast<WiflyControl*>(pNative)->FwClearScript());
}

jboolean Java_biz_bruenn_WiflyLight_WiflyControl_FwLoopOff(JNIEnv* env, jobject ref, jlong pNative, jbyte numLoops)
{
	TRY_CATCH_RETURN_BOOL(reinterpret_cast<WiflyControl*>(pNative)->FwLoopOff(numLoops));
}

jboolean Java_biz_bruenn_WiflyLight_WiflyControl_FwLoopOn(JNIEnv* env, jobject ref, jlong pNative)
{
	TRY_CATCH_RETURN_BOOL(reinterpret_cast<WiflyControl*>(pNative)->FwLoopOn());
}

jboolean Java_biz_bruenn_WiflyLight_WiflyControl_FwSetColor(JNIEnv* env, jobject ref, jlong pNative, jint argb, jint addr)
{
	TRY_CATCH_RETURN_BOOL(reinterpret_cast<WiflyControl*>(pNative)->FwSetFade(argb, 0, addr, false));
}

jboolean Java_biz_bruenn_WiflyLight_WiflyControl_FwSetFade(JNIEnv* env, jobject ref, jlong pNative, jint argb, jint addr, jshort fadeTime)
{
	TRY_CATCH_RETURN_BOOL(reinterpret_cast<WiflyControl*>(pNative)->FwSetFade(argb, fadeTime, addr, false));
}

void Java_biz_bruenn_WiflyLight_WiflyControl_release(JNIEnv* env, jobject ref, jlong pNative)
{
	if((WiflyControl*)pNative == g_pControl) {
		delete g_pControl;
		g_pControl = NULL;
	}
}
}

