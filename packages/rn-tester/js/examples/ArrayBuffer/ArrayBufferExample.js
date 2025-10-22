/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow
 * @format
 */

import manager from '../../../NativeBuffersManager/NativeBuffersManager';
import native from '../../../NativeCxxModuleExample/NativeCxxModuleExample';
import React, {useEffect, useRef, useState} from 'react';
import {
  ActivityIndicator,
  Animated,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from 'react-native';

const SIZE = 1;
const smallBuffer = new ArrayBuffer(4);
const smallBufferView = new Uint8Array(smallBuffer);
smallBufferView[0] = 40;
smallBufferView[1] = 50;
smallBufferView[2] = 60;
smallBufferView[3] = 70;

const ArrayBufferExample = () => {
  const [buffer, setBuffer] = useState(null);
  const [generating, setGenerating] = useState(false);
  const [loadingBase64, setLoadingBase64] = useState(false);
  const [loadingArrayBuffer, setLoadingArrayBuffer] = useState(false);
  const [base64Time, setBase64Time] = useState(null);
  const [arrayBufferTime, setArrayBufferTime] = useState(null);
  const [generateTime, setGenerateTime] = useState(null);
  const progressAnim = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    const eventSubscriptions = [];
    // eventSubscriptions
    // .push
    // manager.onMyString(value => console.log(`onMyString ${value}`)),
    // ();
    // eventSubscriptions.push(
    // manager.onMyBuffer(value => {
    // const view = new Uint8Array(value);
    // console.log(`onMyBuffer ${view}`);
    // }),
    // );
    eventSubscriptions.push(
      native.onData(value => {
        const view = new Uint8Array(value);
        console.log(`onData, length ${view.byteLength}`);
      }),
    );
    // Optionally, clean up subscriptions on unmount
    return () => {
      eventSubscriptions.forEach(sub => sub.remove && sub.remove());
    };
  }, []);

  const generateBuffer = () => {
    setGenerateTime(null);
    setBase64Time(null);
    setArrayBufferTime(null);
    setGenerating(true);
    const start = performance.now();
    const size = SIZE * 1024 * 1024;
    const ab = new ArrayBuffer(size);
    const view = new Uint8Array(ab);
    let i = 0;
    const step = 1024 * 1024; // 1MB chunks
    const animate = () => {
      if (i < size) {
        for (let j = 0; j < step && i < size; j++, i++) {
          view[i] = Math.floor(Math.random() * 256);
        }
        progressAnim.setValue(i / size);
        setTimeout(animate, 0);
      } else {
        setBuffer(ab);
        setGenerating(false);
        const end = performance.now();
        setGenerateTime((end - start).toFixed(2));
      }
    };
    animate();
  };

  const passAsBase64 = () => {
    if (buffer) {
      setBase64Time(null);
      setLoadingBase64(true);
      setTimeout(() => {
        const start = performance.now();
        const base64 = btoa(
          new Uint8Array(buffer).reduce(
            (data, byte) => data + String.fromCharCode(byte),
            '',
          ),
        );
        manager.processBase64(base64);
        manager.processUnion({text: base64});
        manager.processStringStruct({text: base64});
        console.log(`Constants string: ${manager.getConstants().text}`);
        const end = performance.now();
        setBase64Time((end - start).toFixed(2));
        setLoadingBase64(false);
      }, 0);
    }
  };

  const passAsArrayBuffer = () => {
    if (buffer) {
      // setArrayBufferTime(null);
      // setLoadingArrayBuffer(true);
      setTimeout(() => {
        const start = performance.now();
        manager.processBuffer(buffer);
        console.log(`Processing buffer: ${buffer.byteLength}`);
        const b = manager.getBuffer();
        console.log(`Retrieved buffer: ${b.byteLength}`);

        // return fetch('http://localhost:3000/send', {
        //   method: 'POST',
        //   body: smallBuffer,
        // })
        //   .then(response => response.json())
        //   .then(json => {
        //     return json.movies;
        //   })
        //   .catch(error => {
        //     console.error(error);
        //   });

        // manager.processBuffer(new Uint8Array(buffer));
        // manager.processUnsafe(buffer);
        // // manager.processUnion({buffer: buffer});
        // // manager.processArrayBufferStruct({ocb: 'OCB', buffer: buffer});
        // console.log(
        //   `Constants buffer: ${new Uint8Array(manager.getConstants().buffer)}`,
        // );
        // // manager.processArrayOfBuffers([buffer]);

        // native.processBufferUnion({buffer: buffer});
        // native.processBufferStruct({text: 'OCB', value: buffer});
        // console.log(
        //   `Native C++ module buffer: ${new Uint8Array(native.getBufferStruct().value).byteLength}`,
        // );

        // manager.getAsyncBuffer().then(b => {
        //   const view = new Uint8Array(b);
        //   console.log(`getAsyncBuffer, length ${b} ${view.byteLength}`);
        // });

        // manager.processOptionalBuffer(null);
        // native.takingOptionalBuffer(null);
        // const b = native.getOptionalBuffer(7);
        // console.log(
        //   'Native C++ module optional buffer: ' +
        //     (b ? `${new Uint8Array(b).byteLength}` : 'null'),
        // );

        const end = performance.now();
        setArrayBufferTime((end - start).toFixed(2));
        setLoadingArrayBuffer(false);
      }, 0);
    }
  };

  const getTimeInfo = time => {
    const t = parseFloat(time);
    if (t < 100) return {emoji: '🚀', color: '#4caf50'};
    return {emoji: '🐌', color: '#f44336'};
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>ArrayBuffer for TurboModules</Text>
      <TouchableOpacity
        style={[styles.button, generating && styles.buttonDisabled]}
        onPress={generateBuffer}
        disabled={generating}>
        <View style={styles.buttonContent}>
          <Text style={styles.buttonText}>
            {generating ? 'Generating...' : `Generate ${SIZE}MB Buffer`}
          </Text>
          {generating && (
            <ActivityIndicator
              size="small"
              color="white"
              style={styles.indicator}
            />
          )}
        </View>
      </TouchableOpacity>
      {(() => {
        const time = generateTime;
        const info = time
          ? {...getTimeInfo(time), emoji: '✅', color: '#4caf50'}
          : {emoji: '⏱️', color: '#ccc'};
        const displayText = time ? `✓ Done` : '⏱️';
        return (
          <View style={[styles.timeBadge, {backgroundColor: info.color}]}>
            <Text style={[styles.timeText, {color: 'white'}]}>
              {displayText}
            </Text>
          </View>
        );
      })()}
      <TouchableOpacity
        style={[
          styles.button,
          (!buffer || generating || loadingBase64) && styles.buttonDisabled,
        ]}
        onPress={passAsBase64}
        disabled={!buffer || generating || loadingBase64}>
        <View style={styles.buttonContent}>
          <Text style={styles.buttonText}>
            {loadingBase64 ? 'Processing...' : 'Pass as Base64'}
          </Text>
          {loadingBase64 && (
            <ActivityIndicator
              size="small"
              color="white"
              style={styles.indicator}
            />
          )}
        </View>
      </TouchableOpacity>
      {(() => {
        const time = base64Time;
        const info = time ? getTimeInfo(time) : {emoji: '⏱️', color: '#ccc'};
        const displayText = time ? `${info.emoji} ${time} ms` : '⏱️';
        return (
          <View style={[styles.timeBadge, {backgroundColor: info.color}]}>
            <Text style={[styles.timeText, {color: 'white'}]}>
              {displayText}
            </Text>
          </View>
        );
      })()}
      <TouchableOpacity
        style={[
          styles.button,
          (!buffer || generating || loadingArrayBuffer) &&
            styles.buttonDisabled,
        ]}
        onPress={passAsArrayBuffer}
        disabled={!buffer || generating || loadingArrayBuffer}>
        <View style={styles.buttonContent}>
          <Text style={styles.buttonText}>
            {loadingArrayBuffer ? 'Processing...' : 'Pass as ArrayBuffer'}
          </Text>
          {loadingArrayBuffer && (
            <ActivityIndicator
              size="small"
              color="white"
              style={styles.indicator}
            />
          )}
        </View>
      </TouchableOpacity>
      {(() => {
        const time = arrayBufferTime;
        const info = time ? getTimeInfo(time) : {emoji: '⏱️', color: '#ccc'};
        const displayText = time ? `${info.emoji} ${time} ms` : '⏱️';
        return (
          <View style={[styles.timeBadge, {backgroundColor: info.color}]}>
            <Text style={[styles.timeText, {color: 'white'}]}>
              {displayText}
            </Text>
          </View>
        );
      })()}
    </View>
  );
};

const styles = StyleSheet.create({
  button: {
    backgroundColor: '#007AFF',
    borderRadius: 10,
    margin: 10,
    padding: 15,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.3,
    shadowRadius: 3,
    elevation: 5,
    width: 250,
  },
  buttonContent: {
    alignItems: 'center',
    flexDirection: 'row',
    justifyContent: 'center',
  },
  buttonDisabled: {
    backgroundColor: '#ccc',
  },
  buttonText: {
    color: 'white',
    fontSize: 16,
    fontWeight: 'bold',
  },
  container: {
    alignItems: 'center',
    backgroundColor: '#f5f5f5',
    flex: 1,
    justifyContent: 'center',
  },
  indicator: {
    marginLeft: 10,
  },
  progressBar: {
    backgroundColor: '#007AFF',
    height: '100%',
  },
  progressContainer: {
    backgroundColor: '#ddd',
    borderRadius: 10,
    height: 20,
    margin: 10,
    overflow: 'hidden',
    width: '80%',
  },
  timeBadge: {
    borderRadius: 15,
    elevation: 3,
    marginTop: 5,
    marginBottom: 10,
    paddingHorizontal: 10,
    paddingVertical: 5,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 1},
    shadowOpacity: 0.2,
    shadowRadius: 2,
  },
  timeText: {
    fontSize: 14,
    fontWeight: 'bold',
  },
  title: {
    color: '#333',
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
  },
});

export default ArrayBufferExample;
