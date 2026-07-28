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

import {RNTesterThemeContext} from '../../components/RNTesterTheme';
import * as React from 'react';
import {
  type ElementRef,
  useContext,
  useLayoutEffect,
  useRef,
  useState,
} from 'react';
import {Button, StyleSheet, Text, View} from 'react-native';

const FONT_MIN_WIDTH = 80;
const FONT_MAX_WIDTH = 320;
const STEP = 40;

function computeFontSize(contentWidth: number): number {
  return Math.max(12, Math.min(44, Math.round(contentWidth / 8)));
}

function FontScalingExample(): React.Node {
  const theme = useContext(RNTesterThemeContext);
  const boxRef = useRef<?ElementRef<typeof View>>(null);
  const [width, setWidth] = useState(200);
  const [fontSize, setFontSize] = useState(computeFontSize(200));
  const [contentWidth, setContentWidth] = useState<?number>(null);

  useLayoutEffect(() => {
    const box = boxRef.current;
    if (box == null) {
      return;
    }
    const observer = new ResizeObserver(entries => {
      for (const entry of entries) {
        const w = entry.contentRect.width;
        setContentWidth(w);
        setFontSize(computeFontSize(w));
      }
    });
    // $FlowFixMe[incompatible-type]
    observer.observe(box);
    return () => observer.disconnect();
  }, []);

  return (
    <View style={styles.example}>
      <View style={styles.buttonRow}>
        <Button
          title="Narrower"
          disabled={width <= FONT_MIN_WIDTH}
          onPress={() => setWidth(w => Math.max(FONT_MIN_WIDTH, w - STEP))}
        />
        <Button
          title="Wider"
          disabled={width >= FONT_MAX_WIDTH}
          onPress={() => setWidth(w => Math.min(FONT_MAX_WIDTH, w + STEP))}
        />
      </View>
      <View
        ref={boxRef}
        style={[styles.box, {width, borderColor: theme.LabelColor}]}>
        <Text style={[styles.scalingText, {fontSize, color: theme.LabelColor}]}>
          This text grows with its container.
        </Text>
      </View>
      <Text style={[styles.mono, {color: theme.SecondaryLabelColor}]}>
        {contentWidth != null
          ? `content width ${contentWidth.toFixed(1)}pt → font ${fontSize}pt`
          : 'waiting for first observation…'}
      </Text>
    </View>
  );
}

const BOX_MIN_WIDTH = 80;
const BOX_MAX_WIDTH = 300;

type Boxes = {
  contentRect: {x: number, y: number, width: number, height: number},
  contentBox: {inlineSize: number, blockSize: number},
  borderBox: {inlineSize: number, blockSize: number},
  devicePixelBox: {inlineSize: number, blockSize: number},
};

function BoxSizesExample(): React.Node {
  const theme = useContext(RNTesterThemeContext);
  const boxRef = useRef<?ElementRef<typeof View>>(null);
  const [width, setWidth] = useState(160);
  const [padded, setPadded] = useState(false);
  const [bordered, setBordered] = useState(true);
  const [boxes, setBoxes] = useState<?Boxes>(null);

  useLayoutEffect(() => {
    const box = boxRef.current;
    if (box == null) {
      return;
    }
    const observer = new ResizeObserver(entries => {
      for (const entry of entries) {
        const [contentBox] = entry.contentBoxSize;
        const [borderBox] = entry.borderBoxSize;
        const [devicePixelBox] = entry.devicePixelContentBoxSize;
        setBoxes({
          contentRect: {
            x: entry.contentRect.x,
            y: entry.contentRect.y,
            width: entry.contentRect.width,
            height: entry.contentRect.height,
          },
          contentBox: {
            inlineSize: contentBox.inlineSize,
            blockSize: contentBox.blockSize,
          },
          borderBox: {
            inlineSize: borderBox.inlineSize,
            blockSize: borderBox.blockSize,
          },
          devicePixelBox: {
            inlineSize: devicePixelBox.inlineSize,
            blockSize: devicePixelBox.blockSize,
          },
        });
      }
    });
    // $FlowFixMe[incompatible-type]
    observer.observe(box, {box: 'content-box'});
    return () => observer.disconnect();
  }, []);

  const fmt = (n: number) => n.toFixed(1);

  return (
    <View style={styles.example}>
      <View style={styles.buttonRow}>
        <Button
          title="Narrower"
          disabled={width <= BOX_MIN_WIDTH}
          onPress={() => setWidth(w => Math.max(BOX_MIN_WIDTH, w - STEP))}
        />
        <Button
          title="Wider"
          disabled={width >= BOX_MAX_WIDTH}
          onPress={() => setWidth(w => Math.min(BOX_MAX_WIDTH, w + STEP))}
        />
        <Button
          title={padded ? 'No padding' : 'Padding'}
          onPress={() => setPadded(p => !p)}
        />
        <Button
          title={bordered ? 'No border' : 'Border'}
          onPress={() => setBordered(b => !b)}
        />
      </View>
      <View
        ref={boxRef}
        style={[
          styles.sizeBox,
          {
            width,
            padding: padded ? 20 : 0,
            borderWidth: bordered ? 8 : 0,
            borderColor: theme.LabelColor,
          },
        ]}
      />
      {boxes == null ? (
        <Text style={[styles.mono, {color: theme.SecondaryLabelColor}]}>
          waiting for first observation…
        </Text>
      ) : (
        <View style={styles.readout}>
          <Text style={[styles.mono, {color: theme.LabelColor}]}>
            {`border-box:        ${fmt(boxes.borderBox.inlineSize)} × ${fmt(boxes.borderBox.blockSize)}`}
          </Text>
          <Text style={[styles.mono, {color: theme.LabelColor}]}>
            {`content-box:       ${fmt(boxes.contentBox.inlineSize)} × ${fmt(boxes.contentBox.blockSize)}`}
          </Text>
          <Text style={[styles.mono, {color: theme.LabelColor}]}>
            {`device-pixel-box:  ${fmt(boxes.devicePixelBox.inlineSize)} × ${fmt(boxes.devicePixelBox.blockSize)}`}
          </Text>
          <Text style={[styles.mono, {color: theme.SecondaryLabelColor}]}>
            {`contentRect:       x ${fmt(boxes.contentRect.x)}, y ${fmt(boxes.contentRect.y)}`}
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  example: {
    rowGap: 12,
  },
  buttonRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    columnGap: 16,
    rowGap: 8,
  },
  box: {
    minHeight: 96,
    borderRadius: 8,
    justifyContent: 'center',
    padding: 12,
    backgroundColor: 'rgba(99, 102, 241, 0.18)',
  },
  scalingText: {
    fontWeight: '600',
  },
  sizeBox: {
    minHeight: 96,
    width: 160,
    borderRadius: 8,
    backgroundColor: 'rgba(99, 102, 241, 0.18)',
  },
  unmountStage: {
    minHeight: 96,
    justifyContent: 'center',
    alignItems: 'flex-start',
  },
  readout: {
    rowGap: 4,
  },
  mono: {
    fontSize: 14,
    fontFamily: 'Courier',
    fontVariant: ['tabular-nums'],
  },
});

export const framework = 'React';
export const title = 'ResizeObserver';
export const category = 'UI';
export const documentationURL =
  'https://developer.mozilla.org/en-US/docs/Web/API/ResizeObserver';
export const description =
  'API to observe changes to the dimensions of an element and report its ' +
  'content-box, border-box, and device-pixel-content-box sizes.';
export const examples: Array<RNTesterModuleExample> = [
  {
    title: 'Scale text to its container',
    description:
      'Observe the content-box width and derive a font size — the classic ' +
      'MDN/CSS ResizeObserver use case.',
    render: () => <FontScalingExample />,
  },
  {
    title: 'All box sizes from one entry',
    description:
      'A single entry reports border-box, content-box and ' +
      'device-pixel-content-box. Toggle padding and border to see them ' +
      'diverge and the contentRect origin shift.',
    render: () => <BoxSizesExample />,
  },
];
