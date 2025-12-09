/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

import type {RNTesterModuleExample} from '../../types/RNTesterTypes';

import RNTesterText from '../../components/RNTesterText';
import * as React from 'react';
import {Image, StyleSheet, View, Text} from 'react-native';

function BackgroundImageBox({
  style,
  children,
  testID,
}: {
  style?: ViewStyleProp,
  children?: React.Node,
  testID: string,
}) {
  return (
    <View style={[styles.box, style]} testID={testID}>
      {children}
    </View>
  );
}

const SIZE = 110;
const HEART_URL = 'https://static.thenounproject.com/png/432965-200.png';
const REACT_URL = 'https://reactjs.org/logo-og.png';
const URL = `url(${HEART_URL})`;
const REPEAT = 'no-repeat';
const ACCENT = '#4ecdc4';
function Playground() {
  return (
    <View style={styles.col}>
      <RNTesterText
        style={[
          styles.text,
          {fontWeight: 'bold', width: '100%', textAlign: 'center'},
        ]}>
        linear-gradient(...)
      </RNTesterText>
      {/* <View style={styles.row}>
        <View
          style={[
            styles.box,
            {
              backgroundColor: ACCENT,
            },
          ]}
        />
        <RNTesterText style={styles.text}>→</RNTesterText>
        <View
          style={[
            styles.box,
            {
              experimental_backgroundImage:
                'linear-gradient(115deg, black 10%, transparent 30% 70%, black 90%)',
            },
          ]}
        />
        <RNTesterText style={styles.text}>→</RNTesterText>
        <View
          style={[
            styles.box,
            {
              backgroundColor: ACCENT,
              maskImage:
                'linear-gradient(115deg, black 10%, transparent 30% 70%, black 90%)',
              // maskRepeat: REPEAT,
            },
          ]}
        />
      </View>
      <RNTesterText
        style={[
          styles.text,
          {fontWeight: 'bold', width: '100%', textAlign: 'center'},
        ]}>
        url(mask.png)
      </RNTesterText>
      <View style={styles.row}>
        <View
          style={[
            styles.box,
            {
              backgroundColor: ACCENT,
            },
          ]}
        />
        <RNTesterText style={styles.text}>→</RNTesterText>
        <Image source={[{uri: HEART_URL}]} style={styles.box} />
        <RNTesterText style={styles.text}>→</RNTesterText>
        <View
          style={[
            styles.box,
            {
              backgroundColor: ACCENT,
              maskImage: URL,
              maskRepeat: REPEAT,
            },
          ]}
        />
      </View>
      <View style={styles.row}>
        <RNTesterText
          style={[
            styles.text,
            {width: SIZE, fontWeight: 'bold', textAlign: 'center'},
          ]}>
          TEAAXT
        </RNTesterText>
        <RNTesterText style={styles.text}>→</RNTesterText>
        <Image source={[{uri: HEART_URL}]} style={styles.box} />
        <RNTesterText style={styles.text}>→</RNTesterText>
        <RNTesterText
          style={[
            styles.text,
            {
              width: SIZE,
              fontWeight: 'bold',
              textAlign: 'center',
              backgroundColor: ACCENT,
              maskImage:
                'linear-gradient(90deg, black 0%, transparent 50%, black 100%)',
              maskRepeat: REPEAT,
            },
          ]}>
          TEAAXT
        </RNTesterText>
      </View> */}
      <View style={styles.row}>
        <Image source={[{uri: REACT_URL}]} style={[styles.box]}></Image>
        <RNTesterText style={styles.text}>→</RNTesterText>
        <Image source={[{uri: HEART_URL}]} style={styles.box} />
        <RNTesterText style={styles.text}>→</RNTesterText>

        <Image
          source={[{uri: REACT_URL}]}
          style={[
            styles.box,
            {
              backgroundColor: ACCENT,
              maskImage: URL,
              maskRepeat: REPEAT,
            },
          ]}></Image>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  box: {
    width: SIZE,
    height: SIZE,
    justifyContent: 'center',
    alignItems: 'center',
    marginVertical: 10,
  },
  text: {
    fontSize: 24,
  },
  row: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-evenly',
    alignItems: 'center',
  },
  col: {
    alignItems: 'center',
  },
});

export default ({
  title: 'Playground',
  name: 'playground',
  description: 'Test out new features and ideas.',
  render: (): React.Node => <Playground />,
}: RNTesterModuleExample);
