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

const SIZE = 100;
const URL = 'https://static.thenounproject.com/png/432965-200.png';
const REPEAT = 'https://reactnative.dev/img/tiny_logo.png';
const ACCENT = '#4ecdc4';
function Playground() {
  return (
    <View style={styles.row}>
      {/* <View style={styles.col}> */}
      <RNTesterText
        style={[
          styles.text,
          {fontWeight: 'bold', width: '100%', textAlign: 'center'},
        ]}>
        linear-gradient(...)
      </RNTesterText>
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
            // maskRepeat: 'no-repeat',
          },
        ]}
      />
      <RNTesterText
        style={[
          styles.text,
          {fontWeight: 'bold', width: '100%', textAlign: 'center'},
        ]}>
        url(mask.png)
      </RNTesterText>
      <View
        style={[
          styles.box,
          {
            backgroundColor: ACCENT,
          },
        ]}
      />
      <RNTesterText style={styles.text}>→</RNTesterText>
      <Image source={[{uri: URL}]} style={styles.box} />
      <RNTesterText style={styles.text}>→</RNTesterText>
      <View
        style={[
          styles.box,
          {
            backgroundColor: ACCENT,
            maskImage:
              'url(https://static.thenounproject.com/png/432965-200.png)',
            maskRepeat: 'no-repeat',
          },
        ]}
      />
      {/* </View> */}
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
