/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @fantom_flags enableResizeObserverByDefault:true
 * @flow strict-local
 * @format
 */

import '@react-native/fantom/src/setUpDefaultReactNativeEnvironment';

import type {HostInstance} from 'react-native';
import type ResizeObserverType from 'react-native/src/private/webapis/resizeobserver/ResizeObserver';
import type ResizeObserverEntryType from 'react-native/src/private/webapis/resizeobserver/ResizeObserverEntry';
import type ResizeObserverSizeType from 'react-native/src/private/webapis/resizeobserver/ResizeObserverSize';

import ensureInstance from '../../../__tests__/utilities/ensureInstance';
import {createShadowNodeReferenceCountingRef} from '../../../__tests__/utilities/ShadowNodeReferenceCounter';
import * as Fantom from '@react-native/fantom';
import * as React from 'react';
import {createRef} from 'react';
import {View} from 'react-native';
import setUpResizeObserver from 'react-native/src/private/setup/setUpResizeObserver';
import ReactNativeElement from 'react-native/src/private/webapis/dom/nodes/ReactNativeElement';
import DOMRectReadOnly from 'react-native/src/private/webapis/geometry/DOMRectReadOnly';

declare const ResizeObserver: Class<ResizeObserverType>;
declare const ResizeObserverEntry: Class<ResizeObserverEntryType>;
declare const ResizeObserverSize: Class<ResizeObserverSizeType>;

type ResizeObserverMockCallback = JestMockFn<
  [ReadonlyArray<ResizeObserverEntry>, ResizeObserver],
  unknown,
>;

setUpResizeObserver();

function ensureReactNativeElement(value: unknown): ReactNativeElement {
  return ensureInstance(value, ReactNativeElement);
}

function expectEntrySizes(
  entry: ResizeObserverEntry,
  expected: {
    contentWidth: number,
    contentHeight: number,
    borderWidth: number,
    borderHeight: number,
    contentX?: number,
    contentY?: number,
    devicePixelWidth?: number,
    devicePixelHeight?: number,
  },
): void {
  expect(entry.contentRect).toBeInstanceOf(DOMRectReadOnly);
  expect(entry.contentRect.x).toBe(expected.contentX ?? 0);
  expect(entry.contentRect.y).toBe(expected.contentY ?? 0);
  expect(entry.contentRect.width).toBe(expected.contentWidth);
  expect(entry.contentRect.height).toBe(expected.contentHeight);

  expect(entry.contentBoxSize).toHaveLength(1);
  expect(entry.contentBoxSize[0].inlineSize).toBe(expected.contentWidth);
  expect(entry.contentBoxSize[0].blockSize).toBe(expected.contentHeight);

  expect(entry.borderBoxSize).toHaveLength(1);
  expect(entry.borderBoxSize[0].inlineSize).toBe(expected.borderWidth);
  expect(entry.borderBoxSize[0].blockSize).toBe(expected.borderHeight);

  if (expected.devicePixelWidth != null && expected.devicePixelHeight != null) {
    expect(entry.devicePixelContentBoxSize).toHaveLength(1);
    expect(entry.devicePixelContentBoxSize[0].inlineSize).toBe(
      expected.devicePixelWidth,
    );
    expect(entry.devicePixelContentBoxSize[0].blockSize).toBe(
      expected.devicePixelHeight,
    );
  }
}

describe('ResizeObserver', () => {
  let observer: ResizeObserver;

  afterEach(() => {
    Fantom.runTask(() => {
      if (observer != null) {
        observer.disconnect();
      }
    });
  });

  describe('constructor(callback)', () => {
    it('should throw if `callback` is not provided', () => {
      expect(() => {
        // $FlowExpectedError[incompatible-type]
        return new ResizeObserver();
      }).toThrow(
        "Failed to construct 'ResizeObserver': 1 argument required, but only 0 present.",
      );
    });

    it('should throw if `callback` is not a function', () => {
      expect(() => {
        // $FlowExpectedError[incompatible-type]
        return new ResizeObserver('not a function!');
      }).toThrow(
        "Failed to construct 'ResizeObserver': parameter 1 is not of type 'Function'.",
      );
    });
  });

  describe('observe(target, options)', () => {
    it('should throw if `target` is null or undefined', () => {
      observer = new ResizeObserver(() => {});

      expect(() => {
        // $FlowExpectedError[incompatible-type]
        observer.observe(null);
      }).toThrow(
        "Failed to execute 'observe' on 'ResizeObserver': parameter 1 is null or undefined.",
      );

      expect(() => {
        // $FlowExpectedError[incompatible-type]
        observer.observe(undefined);
      }).toThrow(
        "Failed to execute 'observe' on 'ResizeObserver': parameter 1 is null or undefined.",
      );
    });

    it('should throw if `target` is not a `ReactNativeElement`', () => {
      observer = new ResizeObserver(() => {});
      expect(() => {
        // $FlowExpectedError[incompatible-type]
        observer.observe('something');
      }).toThrow(
        "Failed to execute 'observe' on 'ResizeObserver': parameter 1 is not of type 'ReactNativeElement'.",
      );
    });

    it('should throw if `box` is not a valid enum value', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();
      Fantom.runTask(() => {
        root.render(<View style={{width: 10, height: 10}} ref={nodeRef} />);
      });
      const node = ensureReactNativeElement(nodeRef.current);

      observer = new ResizeObserver(() => {});
      expect(() => {
        // $FlowExpectedError[incompatible-type]
        observer.observe(node, {box: 'margin-box'});
      }).toThrow(
        "Failed to execute 'observe' on 'ResizeObserver': Failed to read the 'box' property from 'ResizeObserverOptions': The provided value 'margin-box' is not a valid enum value of type ResizeObserverBoxOptions.",
      );
    });

    it('should accept valid `box` option values', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();
      Fantom.runTask(() => {
        root.render(<View style={{width: 10, height: 10}} ref={nodeRef} />);
      });
      const node = ensureReactNativeElement(nodeRef.current);

      expect(() => {
        observer = new ResizeObserver(() => {});
        observer.observe(node, {box: 'content-box'});
        observer.unobserve(node);
        observer.observe(node, {box: 'border-box'});
        observer.unobserve(node);
        observer.observe(node, {box: 'device-pixel-content-box'});
      }).not.toThrow();
    });

    it('should ignore calls to observe disconnected targets', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();
      Fantom.runTask(() => {
        root.render(<View style={{width: 10, height: 10}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);

      Fantom.runTask(() => {
        root.render(<></>);
      });
      expect(node.isConnected).toBe(false);

      const callback = jest.fn();
      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        expect(() => {
          observer.observe(node);
        }).not.toThrow();
      });

      expect(callback).not.toHaveBeenCalled();
    });

    it('should deliver an initial observation for a sized target', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot({devicePixelRatio: 2});

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });

      expect(callback).toHaveBeenCalledTimes(1);
      const [entries, reportedObserver] = callback.mock.lastCall;
      expect(entries).toHaveLength(1);
      expect(entries[0]).toBeInstanceOf(ResizeObserverEntry);
      expect(entries[0].target).toBe(node);
      expect(reportedObserver).toBe(observer);
      expectEntrySizes(entries[0], {
        contentWidth: 100,
        contentHeight: 50,
        borderWidth: 100,
        borderHeight: 50,
        devicePixelWidth: 200,
        devicePixelHeight: 100,
      });
    });

    it('should deliver an initial observation for a zero-sized target', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 0, height: 0}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });

      expect(callback).toHaveBeenCalledTimes(1);
      const [entries] = callback.mock.lastCall;
      expectEntrySizes(entries[0], {
        contentWidth: 0,
        contentHeight: 0,
        borderWidth: 0,
        borderHeight: 0,
      });
    });

    it('should report content and border box sizes with padding and border', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View
            style={{
              width: 100,
              height: 80,
              padding: 10,
              borderWidth: 5,
            }}
            ref={nodeRef}
          />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });

      expect(callback).toHaveBeenCalledTimes(1);
      const [entries] = callback.mock.lastCall;
      // border-box: 100x80
      // content insets = padding(10) + border(5) on each side → content 70x50
      // contentRect origin is padding only → (10, 10)
      expectEntrySizes(entries[0], {
        contentWidth: 70,
        contentHeight: 50,
        borderWidth: 100,
        borderHeight: 80,
        contentX: 10,
        contentY: 10,
      });
    });

    it('should report device-pixel-content-box observations using the root pixel ratio', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot({devicePixelRatio: 3});

      Fantom.runTask(() => {
        root.render(<View style={{width: 10.4, height: 10.6}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node, {box: 'device-pixel-content-box'});
      });

      expect(callback).toHaveBeenCalledTimes(1);
      const [entries] = callback.mock.lastCall;
      expect(entries[0].devicePixelContentBoxSize[0].inlineSize).toBe(
        Math.round(10.4 * 3),
      );
      expect(entries[0].devicePixelContentBoxSize[0].blockSize).toBe(
        Math.round(10.6 * 3),
      );
    });

    it('should ignore subsequent observe calls for the same target and box', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        observer.observe(node);
        observer.observe(node, {box: 'content-box'});
      });
      expect(callback).toHaveBeenCalledTimes(1);
    });

    it('should re-deliver when re-observing the same target with a different box', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View
            style={{width: 100, height: 80, padding: 10, borderWidth: 5}}
            ref={nodeRef}
          />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node, {box: 'content-box'});
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        observer.observe(node, {box: 'border-box'});
      });
      expect(callback).toHaveBeenCalledTimes(2);

      const [entries] = callback.mock.lastCall;
      expect(entries[0].borderBoxSize[0].inlineSize).toBe(100);
      expect(entries[0].borderBoxSize[0].blockSize).toBe(80);
      expect(entries[0].contentBoxSize[0].inlineSize).toBe(70);
      expect(entries[0].contentBoxSize[0].blockSize).toBe(50);
    });

    it('should report size updates for the observed target', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 200, height: 75}} ref={nodeRef} />,
        );
      });

      expect(callback).toHaveBeenCalledTimes(2);
      const [entries] = callback.mock.lastCall;
      expectEntrySizes(entries[0], {
        contentWidth: 200,
        contentHeight: 75,
        borderWidth: 200,
        borderHeight: 75,
      });
    });

    it('should not report updates that do not change the observed box', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 80, backgroundColor: 'red'}}
            ref={nodeRef}
          />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node, {box: 'border-box'});
      });
      expect(callback).toHaveBeenCalledTimes(1);

      // Non-layout style change should not notify.
      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 80, backgroundColor: 'blue'}}
            ref={nodeRef}
          />,
        );
      });
      expect(callback).toHaveBeenCalledTimes(1);

      // Padding changes content-box but not border-box.
      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{
              width: 100,
              height: 80,
              padding: 10,
              backgroundColor: 'blue',
            }}
            ref={nodeRef}
          />,
        );
      });
      expect(callback).toHaveBeenCalledTimes(1);
    });

    it('should treat the default (no `box` option) observation as content-box, not border-box', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 80, padding: 10, borderWidth: 5}}
            ref={nodeRef}
          />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 70,
        contentHeight: 50,
        borderWidth: 100,
        borderHeight: 80,
        contentX: 10,
        contentY: 10,
      });

      // Grow the border-box while growing the border by the same amount
      // (padding unchanged), so the content box stays exactly the same:
      // border-box goes 100x80 -> 110x90, but content-box stays 70x50. If
      // the default box were border-box, this would fire.
      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 110, height: 90, padding: 10, borderWidth: 10}}
            ref={nodeRef}
          />,
        );
      });
      expect(callback).toHaveBeenCalledTimes(1);

      // A genuine content-box change still fires, confirming the observer
      // is live and this isn't merely stuck.
      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 130, height: 90, padding: 10, borderWidth: 10}}
            ref={nodeRef}
          />,
        );
      });
      expect(callback).toHaveBeenCalledTimes(2);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 90,
        contentHeight: 50,
        borderWidth: 130,
        borderHeight: 90,
        contentX: 10,
        contentY: 10,
      });
    });

    it('should report zero sizes when the target becomes hidden', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 50, display: 'none'}}
            ref={nodeRef}
          />,
        );
      });

      expect(callback).toHaveBeenCalledTimes(2);
      const [entries] = callback.mock.lastCall;
      expectEntrySizes(entries[0], {
        contentWidth: 0,
        contentHeight: 0,
        borderWidth: 0,
        borderHeight: 0,
      });
    });

    it('should report a content-box change when padding is added', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 80}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node, {box: 'content-box'});
      });
      expect(callback).toHaveBeenCalledTimes(1);

      // Adding padding shrinks the content box (border box is unchanged), so a
      // content-box observation must fire. This is the complement of the
      // border-box case, where the same padding change does not fire.
      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 80, padding: 10}}
            ref={nodeRef}
          />,
        );
      });

      expect(callback).toHaveBeenCalledTimes(2);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 80,
        contentHeight: 60,
        borderWidth: 100,
        borderHeight: 80,
        contentX: 10,
        contentY: 10,
      });
    });

    it('should report border-box changes when observing the border box', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 80, borderWidth: 5}}
            ref={nodeRef}
          />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node, {box: 'border-box'});
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 140, height: 80, borderWidth: 5}}
            ref={nodeRef}
          />,
        );
      });

      expect(callback).toHaveBeenCalledTimes(2);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 130, // 140 - 2 * 5
        contentHeight: 70, // 80 - 2 * 5
        borderWidth: 140,
        borderHeight: 80,
      });
    });

    it('should not deliver a callback for transforms (layout size unchanged)', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 100}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      // A CSS transform does not change the layout box, so no observation
      // fires (matches the spec: observations are not triggered by transforms).
      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 100, transform: [{scale: 1.5}]}}
            ref={nodeRef}
          />,
        );
      });

      expect(callback).toHaveBeenCalledTimes(1);
    });

    it('should re-deliver the size when a hidden target is shown again', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        root.render(
          <View
            key="target"
            style={{width: 100, height: 50, display: 'none'}}
            ref={nodeRef}
          />,
        );
      });
      expect(callback).toHaveBeenCalledTimes(2);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 0,
        contentHeight: 0,
        borderWidth: 0,
        borderHeight: 0,
      });

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });
      expect(callback).toHaveBeenCalledTimes(3);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 100,
        contentHeight: 50,
        borderWidth: 100,
        borderHeight: 50,
      });
    });

    // Covers the EmptyLayoutMetrics path: a mounted child under a
    // display:none ancestor has no reliable relative box, so we must not
    // deliver (including no 0x0). When the ancestor is shown, lastReportedSize_
    // was reset and the next successful read is treated as an initial delivery.
    it('should not deliver while an ancestor is display:none, then deliver when ancestor is shown', () => {
      const childRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="parent" style={{display: 'none'}}>
            <View key="child" style={{width: 100, height: 50}} ref={childRef} />
          </View>,
        );
      });

      const child = ensureReactNativeElement(childRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(child);
      });

      // EmptyLayoutMetrics path: no reliable box — no callback (including no 0x0).
      expect(callback).not.toHaveBeenCalled();

      Fantom.runTask(() => {
        root.render(
          <View key="parent">
            <View key="child" style={{width: 100, height: 50}} ref={childRef} />
          </View>,
        );
      });

      expect(callback).toHaveBeenCalledTimes(1);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 100,
        contentHeight: 50,
        borderWidth: 100,
        borderHeight: 50,
      });
    });

    it('should deliver to multiple observers watching the same target', () => {
      const nodeRef = createRef<HostInstance>();
      let observer1: ResizeObserver;
      let observer2: ResizeObserver;
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 120, height: 60}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback1 = jest.fn();
      const callback2 = jest.fn();

      Fantom.runTask(() => {
        observer1 = new ResizeObserver(callback1);
        observer2 = new ResizeObserver(callback2);
        observer1.observe(node);
        observer2.observe(node);
      });

      expect(callback1).toHaveBeenCalledTimes(1);
      expect(callback2).toHaveBeenCalledTimes(1);
      expect(callback1.mock.lastCall[0][0].target).toBe(node);
      expect(callback2.mock.lastCall[0][0].target).toBe(node);

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 200, height: 60}} ref={nodeRef} />,
        );
      });

      expect(callback1).toHaveBeenCalledTimes(2);
      expect(callback2).toHaveBeenCalledTimes(2);

      Fantom.runTask(() => {
        observer1.disconnect();
        observer2.disconnect();
      });
    });

    it('should deliver a final 0x0 observation when the target is removed from the tree', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      // Unmount the observed target *without* calling `unobserve`.
      Fantom.runTask(() => {
        root.render(<></>);
      });

      // Matches the Web: removal from the tree fires one final notification
      // with a 0x0 box.
      expect(callback).toHaveBeenCalledTimes(2);
      const [entries] = callback.mock.lastCall;
      expect(entries).toHaveLength(1);
      expect(entries[0].target).toBe(node);
      expectEntrySizes(entries[0], {
        contentWidth: 0,
        contentHeight: 0,
        borderWidth: 0,
        borderHeight: 0,
      });

      // No further notifications while it stays detached.
      Fantom.runTask(() => {
        root.render(<View key="other" style={{width: 10, height: 10}} />);
      });
      expect(callback).toHaveBeenCalledTimes(2);
    });

    it('should not re-deliver on the original observation when a removed target is remounted as a new host instance', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        root.render(<></>);
      });
      expect(callback).toHaveBeenCalledTimes(2);
      expectEntrySizes(callback.mock.lastCall[0][0], {
        contentWidth: 0,
        contentHeight: 0,
        borderWidth: 0,
        borderHeight: 0,
      });

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 120, height: 60}} ref={nodeRef} />,
        );
      });

      // React remount allocates a new ShadowNodeFamily. Observations are keyed
      // by family, so the original observation stays detached — unlike
      // `display: 'none'`, which keeps the same family and re-delivers when
      // shown again. Web-style reinsertion of the *same* Element is not
      // expressible via React remount.
      expect(callback).toHaveBeenCalledTimes(2);
      const remountedNode = ensureReactNativeElement(nodeRef.current);
      expect(remountedNode).not.toBe(node);

      Fantom.runTask(() => {
        observer.observe(remountedNode);
      });
      expect(callback).toHaveBeenCalledTimes(3);
      const [entries] = callback.mock.lastCall;
      expect(entries).toHaveLength(1);
      expect(entries[0].target).toBe(remountedNode);
      expectEntrySizes(entries[0], {
        contentWidth: 120,
        contentHeight: 60,
        borderWidth: 120,
        borderHeight: 60,
      });
    });

    it('should not notify when only an ancestor layout changes but the observed box is unchanged', () => {
      const childRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="parent" style={{width: 200, height: 200}}>
            <View key="child" style={{width: 50, height: 50}} ref={childRef} />
          </View>,
        );
      });

      const child = ensureReactNativeElement(childRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(child, {box: 'border-box'});
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        root.render(
          <View key="parent" style={{width: 300, height: 200}}>
            <View key="child" style={{width: 50, height: 50}} ref={childRef} />
          </View>,
        );
      });

      expect(callback).toHaveBeenCalledTimes(1);
    });

    it('should deliver initial observation for a target observed inside another observer callback on a later event-loop step', () => {
      const nodeARef = createRef<HostInstance>();
      const nodeBRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <>
            <View key="a" style={{width: 100, height: 50}} ref={nodeARef} />
            <View key="b" style={{width: 80, height: 40}} ref={nodeBRef} />
          </>,
        );
      });

      const nodeA = ensureReactNativeElement(nodeARef.current);
      const nodeB = ensureReactNativeElement(nodeBRef.current);
      const callbackB = jest.fn();
      let observerB: ResizeObserver;

      const callbackA: ResizeObserverMockCallback = jest.fn(() => {
        expect(callbackB).not.toHaveBeenCalled();
        observerB = new ResizeObserver(callbackB);
        observerB.observe(nodeB);
        expect(callbackB).not.toHaveBeenCalled();
      });

      Fantom.runTask(() => {
        observer = new ResizeObserver(callbackA);
        observer.observe(nodeA);
      });

      expect(callbackA).toHaveBeenCalledTimes(1);
      expect(callbackB).toHaveBeenCalledTimes(1);
      expectEntrySizes(callbackB.mock.lastCall[0][0], {
        contentWidth: 80,
        contentHeight: 40,
        borderWidth: 80,
        borderHeight: 40,
      });

      Fantom.runTask(() => {
        observerB.disconnect();
      });
    });

    describe('multiple surfaces', () => {
      it('should deliver resize observations independently per surface', () => {
        const node1Ref = createRef<HostInstance>();
        const node2Ref = createRef<HostInstance>();
        const root1 = Fantom.createRoot();
        const root2 = Fantom.createRoot();
        const callback1 = jest.fn();
        const callback2 = jest.fn();
        let observer1: ResizeObserver;
        let observer2: ResizeObserver;

        Fantom.runTask(() => {
          root1.render(
            <View style={{width: 100, height: 50}} ref={node1Ref} />,
          );
          root2.render(
            <View style={{width: 200, height: 80}} ref={node2Ref} />,
          );
        });

        const node1 = ensureReactNativeElement(node1Ref.current);
        const node2 = ensureReactNativeElement(node2Ref.current);

        Fantom.runTask(() => {
          observer1 = new ResizeObserver(callback1);
          observer2 = new ResizeObserver(callback2);
          observer1.observe(node1);
          observer2.observe(node2);
        });

        expect(callback1).toHaveBeenCalledTimes(1);
        expect(callback2).toHaveBeenCalledTimes(1);
        expectEntrySizes(callback1.mock.lastCall[0][0], {
          contentWidth: 100,
          contentHeight: 50,
          borderWidth: 100,
          borderHeight: 50,
        });
        expectEntrySizes(callback2.mock.lastCall[0][0], {
          contentWidth: 200,
          contentHeight: 80,
          borderWidth: 200,
          borderHeight: 80,
        });

        Fantom.runTask(() => {
          root1.render(
            <View style={{width: 150, height: 50}} ref={node1Ref} />,
          );
          root2.render(
            <View style={{width: 200, height: 120}} ref={node2Ref} />,
          );
        });

        expect(callback1).toHaveBeenCalledTimes(2);
        expect(callback2).toHaveBeenCalledTimes(2);
        expect(callback1.mock.lastCall[0][0].contentRect.width).toBe(150);
        expect(callback2.mock.lastCall[0][0].contentRect.height).toBe(120);

        Fantom.runTask(() => {
          observer1.disconnect();
          observer2.disconnect();
        });
      });
    });

    describe('multiple commits within a single tick', () => {
      it('should coalesce multiple commits to the same target into a single callback reporting the final size', () => {
        const nodeRef = createRef<HostInstance>();
        const root = Fantom.createRoot();

        Fantom.runTask(() => {
          root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
        });

        const node = ensureReactNativeElement(nodeRef.current);
        const callback = jest.fn();

        Fantom.runTask(() => {
          observer = new ResizeObserver(callback);
          observer.observe(node);
        });
        expect(callback).toHaveBeenCalledTimes(1);

        // Two commits land in this single tick, before the resize-observer
        // step runs (which happens once per tick, not once per commit).
        Fantom.runTask(() => {
          root.render(<View style={{width: 150, height: 60}} ref={nodeRef} />);
          root.render(<View style={{width: 200, height: 80}} ref={nodeRef} />);
        });

        expect(callback).toHaveBeenCalledTimes(2);
        const [entries] = callback.mock.lastCall;
        expect(entries).toHaveLength(1);
        expectEntrySizes(entries[0], {
          contentWidth: 200,
          contentHeight: 80,
          borderWidth: 200,
          borderHeight: 80,
        });
      });

      it('should coalesce commits to two different observed targets into a single callback carrying both entries in observation order', () => {
        const node1Ref = createRef<HostInstance>();
        const node2Ref = createRef<HostInstance>();
        const root = Fantom.createRoot();

        Fantom.runTask(() => {
          root.render(
            <>
              <View
                key="node1"
                style={{width: 40, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 80, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
        });

        const node1 = ensureReactNativeElement(node1Ref.current);
        const node2 = ensureReactNativeElement(node2Ref.current);
        const callback = jest.fn();

        Fantom.runTask(() => {
          observer = new ResizeObserver(callback);
          observer.observe(node1);
          observer.observe(node2);
        });
        expect(callback).toHaveBeenCalledTimes(1);

        // First commit resizes node1, second (separate) commit resizes
        // node2, both within the same tick.
        Fantom.runTask(() => {
          root.render(
            <>
              <View
                key="node1"
                style={{width: 60, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 80, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
          root.render(
            <>
              <View
                key="node1"
                style={{width: 60, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 100, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
        });

        expect(callback).toHaveBeenCalledTimes(2);
        const [entries] = callback.mock.lastCall;
        expect(entries).toHaveLength(2);
        // Observation order (node1 observed before node2), not commit order.
        expect(entries[0].target).toBe(node1);
        expect(entries[0].contentRect.width).toBe(60);
        expect(entries[1].target).toBe(node2);
        expect(entries[1].contentRect.width).toBe(100);
      });
    });

    it('should report updates to the right observers', () => {
      const node1Ref = createRef<HostInstance>();
      const node2Ref = createRef<HostInstance>();
      let observer1: ResizeObserver;
      let observer2: ResizeObserver;

      const root = Fantom.createRoot();
      Fantom.runTask(() => {
        root.render(
          <>
            <View key="node1" style={{width: 50, height: 50}} ref={node1Ref} />
            <View
              key="node2"
              style={{width: 200, height: 100}}
              ref={node2Ref}
            />
          </>,
        );
      });

      const node1 = ensureReactNativeElement(node1Ref.current);
      const node2 = ensureReactNativeElement(node2Ref.current);
      const callback1 = jest.fn();
      const callback2 = jest.fn();

      Fantom.runTask(() => {
        observer1 = new ResizeObserver(callback1);
        observer1.observe(node1);
        observer1.observe(node2);

        observer2 = new ResizeObserver(callback2);
        observer2.observe(node2);
      });

      expect(callback1).toHaveBeenCalledTimes(1);
      expect(callback2).toHaveBeenCalledTimes(1);

      const [entries1, reportedObserver1] = callback1.mock.lastCall;
      expect(reportedObserver1).toBe(observer1);
      expect(entries1).toHaveLength(2);
      expect(entries1[0].target).toBe(node1);
      expect(entries1[1].target).toBe(node2);

      const [entries2, reportedObserver2] = callback2.mock.lastCall;
      expect(reportedObserver2).toBe(observer2);
      expect(entries2).toHaveLength(1);
      expect(entries2[0].target).toBe(node2);

      Fantom.runTask(() => {
        root.render(
          <>
            <View key="node1" style={{width: 60, height: 50}} ref={node1Ref} />
            <View
              key="node2"
              style={{width: 200, height: 100}}
              ref={node2Ref}
            />
          </>,
        );
      });

      expect(callback1).toHaveBeenCalledTimes(2);
      expect(callback2).toHaveBeenCalledTimes(1);
      const [updateEntries] = callback1.mock.lastCall;
      expect(updateEntries).toHaveLength(1);
      expect(updateEntries[0].target).toBe(node1);
      expect(updateEntries[0].contentRect.width).toBe(60);

      Fantom.runTask(() => {
        observer1.disconnect();
        observer2.disconnect();
      });
    });

    describe('observing multiple targets in the same observer', () => {
      it('should report changes for disjoint observations in observation order', () => {
        const node1Ref = createRef<HostInstance>();
        const node2Ref = createRef<HostInstance>();
        const root = Fantom.createRoot();

        Fantom.runTask(() => {
          root.render(
            <>
              <View
                key="node1"
                style={{width: 40, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 80, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
        });

        const node1 = ensureReactNativeElement(node1Ref.current);
        const node2 = ensureReactNativeElement(node2Ref.current);
        const callback = jest.fn();

        Fantom.runTask(() => {
          observer = new ResizeObserver(callback);
          observer.observe(node2);
          observer.observe(node1);
        });

        expect(callback).toHaveBeenCalledTimes(1);
        const [entries] = callback.mock.lastCall;
        expect(entries.map(entry => entry.target)).toEqual([node2, node1]);

        Fantom.runTask(() => {
          root.render(
            <>
              <View
                key="node1"
                style={{width: 40, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 90, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
        });

        expect(callback).toHaveBeenCalledTimes(2);
        const [updateEntries] = callback.mock.lastCall;
        expect(updateEntries).toHaveLength(1);
        expect(updateEntries[0].target).toBe(node2);
        expect(updateEntries[0].contentRect.width).toBe(90);
      });
    });

    describe('cross-observer callback ordering', () => {
      it('should invoke callbacks in observe() order across different observers', () => {
        const node1Ref = createRef<HostInstance>();
        const node2Ref = createRef<HostInstance>();
        const root = Fantom.createRoot();

        Fantom.runTask(() => {
          root.render(
            <>
              <View
                key="node1"
                style={{width: 40, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 80, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
        });

        const node1 = ensureReactNativeElement(node1Ref.current);
        const node2 = ensureReactNativeElement(node2Ref.current);

        const callOrder: Array<string> = [];
        const callbackA: ResizeObserverMockCallback = jest.fn(() =>
          callOrder.push('A'),
        );
        const callbackB: ResizeObserverMockCallback = jest.fn(() =>
          callOrder.push('B'),
        );
        let observerA: ResizeObserver;
        let observerB: ResizeObserver;

        Fantom.runTask(() => {
          observerA = new ResizeObserver(callbackA);
          observerB = new ResizeObserver(callbackB);
          observerA.observe(node1);
          observerB.observe(node2);
        });

        expect(callOrder).toEqual(['A', 'B']);
        callOrder.length = 0;

        Fantom.runTask(() => {
          root.render(
            <>
              <View
                key="node1"
                style={{width: 50, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 90, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
        });

        expect(callOrder).toEqual(['A', 'B']);

        Fantom.runTask(() => {
          observerA.disconnect();
          observerB.disconnect();
        });
      });

      it('should order callbacks by first-observe() order, not construction order', () => {
        const node1Ref = createRef<HostInstance>();
        const node2Ref = createRef<HostInstance>();
        const root = Fantom.createRoot();

        Fantom.runTask(() => {
          root.render(
            <>
              <View
                key="node1"
                style={{width: 40, height: 40}}
                ref={node1Ref}
              />
              <View
                key="node2"
                style={{width: 80, height: 80}}
                ref={node2Ref}
              />
            </>,
          );
        });

        const node1 = ensureReactNativeElement(node1Ref.current);
        const node2 = ensureReactNativeElement(node2Ref.current);

        const callOrder: Array<string> = [];
        const callbackA: ResizeObserverMockCallback = jest.fn(() =>
          callOrder.push('A'),
        );
        const callbackB: ResizeObserverMockCallback = jest.fn(() =>
          callOrder.push('B'),
        );
        let observerA: ResizeObserver;
        let observerB: ResizeObserver;

        Fantom.runTask(() => {
          // `observerA` is constructed first, but `observerB` calls
          // `observe()` (and so registers with the manager) first.
          // Cross-observer ordering is keyed off registration order, which
          // happens lazily at the first `observe()` call, not at
          // construction time — so B is expected to run before A.
          observerA = new ResizeObserver(callbackA);
          observerB = new ResizeObserver(callbackB);
          observerB.observe(node2);
          observerA.observe(node1);
        });

        expect(callOrder).toEqual(['B', 'A']);

        Fantom.runTask(() => {
          observerA.disconnect();
          observerB.disconnect();
        });
      });
    });

    describe('memory handling', () => {
      it('should not retain initial children of observed targets', () => {
        const root = Fantom.createRoot();
        observer = new ResizeObserver(() => {});

        const [getReferenceCount, ref] = createShadowNodeReferenceCountingRef();

        const observeRef: React.RefSetter<
          React.ElementRef<typeof View>,
        > = instance => {
          const element = ensureReactNativeElement(instance);
          observer.observe(element);
          return () => {
            observer.unobserve(element);
          };
        };

        function Observe({children}: Readonly<{children?: React.Node}>) {
          return (
            <View style={{width: 100, height: 100}} ref={observeRef}>
              {children}
            </View>
          );
        }

        Fantom.runTask(() => {
          root.render(
            <Observe>
              <View ref={ref} />
            </Observe>,
          );
        });

        expect(getReferenceCount()).toBeGreaterThan(0);

        Fantom.runTask(() => {
          root.render(<Observe />);
        });

        expect(getReferenceCount()).toBe(0);
      });
    });
  });

  describe('callback error handling', () => {
    it('should not prevent other observers from receiving entries when a callback throws', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback1: ResizeObserverMockCallback = jest.fn(() => {
        throw new Error('observer 1 failed');
      });
      const callback2 = jest.fn();
      let observer1: ResizeObserver;
      let observer2: ResizeObserver;

      const originalConsoleError = console.error;
      const errorSpy = jest.fn();
      // $FlowExpectedError[cannot-write]
      console.error = errorSpy;

      try {
        Fantom.runTask(() => {
          observer1 = new ResizeObserver(callback1);
          observer2 = new ResizeObserver(callback2);
          observer1.observe(node);
          observer2.observe(node);
        });

        expect(callback1).toHaveBeenCalledTimes(1);
        expect(callback2).toHaveBeenCalledTimes(1);
        expect(errorSpy).toHaveBeenCalled();
      } finally {
        // $FlowExpectedError[cannot-write]
        console.error = originalConsoleError;
        Fantom.runTask(() => {
          observer1.disconnect();
          observer2.disconnect();
        });
      }
    });
  });

  describe('callback re-entrancy', () => {
    it('should allow an observer to disconnect itself from within its own callback without throwing, and not be re-notified afterwards', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      let selfObserver: ResizeObserver;
      const callback: ResizeObserverMockCallback = jest.fn(() => {
        selfObserver.disconnect();
      });

      Fantom.runTask(() => {
        selfObserver = new ResizeObserver(callback);
        selfObserver.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      expect(() => {
        Fantom.runTask(() => {
          root.render(<View style={{width: 200, height: 60}} ref={nodeRef} />);
        });
      }).not.toThrow();

      // The observer disconnected itself during the initial delivery, so the
      // subsequent resize is never reported.
      expect(callback).toHaveBeenCalledTimes(1);
    });

    it("should drop a not-yet-invoked observer's entries without error when an earlier callback disconnects it in the same batch", () => {
      const node1Ref = createRef<HostInstance>();
      const node2Ref = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <>
            <View key="node1" style={{width: 40, height: 40}} ref={node1Ref} />
            <View key="node2" style={{width: 80, height: 80}} ref={node2Ref} />
          </>,
        );
      });

      const node1 = ensureReactNativeElement(node1Ref.current);
      const node2 = ensureReactNativeElement(node2Ref.current);

      let observerA: ResizeObserver;
      let observerB: ResizeObserver;
      const callbackB = jest.fn();
      const callbackA: ResizeObserverMockCallback = jest.fn(() => {
        // A is invoked before B (per observe() order); disconnect B before
        // its own turn in this same delivery batch.
        observerB.disconnect();
      });

      expect(() => {
        Fantom.runTask(() => {
          observerA = new ResizeObserver(callbackA);
          observerB = new ResizeObserver(callbackB);
          observerA.observe(node1);
          observerB.observe(node2);
        });
      }).not.toThrow();

      expect(callbackA).toHaveBeenCalledTimes(1);
      // B's entries were already computed for this batch, but its
      // registration was removed before its turn — doNotifyResizeObservers
      // must skip it silently rather than throw or invoke a stale callback.
      expect(callbackB).not.toHaveBeenCalled();

      Fantom.runTask(() => {
        observerA.disconnect();
      });
    });
  });

  describe('resize-loop guardrail', () => {
    it('should not hang when a callback resizes its own observed target, and should deliver the follow-up resize on a later tick', () => {
      const nodeRef = createRef<HostInstance>();
      const setWidthRef: {current: ?(number) => void} = {current: null};
      const root = Fantom.createRoot();

      function SelfResizingBox() {
        const [width, setWidth] = React.useState(100);
        setWidthRef.current = setWidth;
        return <View style={{width, height: 50}} ref={nodeRef} />;
      }

      Fantom.runTask(() => {
        root.render(<SelfResizingBox />);
      });

      const node = ensureReactNativeElement(nodeRef.current);

      // Per finding H2, RN does not run the spec's synchronous re-layout /
      // re-broadcast loop (and never emits "ResizeObserver loop completed
      // with undelivered notifications."). Resizing the very target we're
      // observing, from inside our own callback, must not hang, and the
      // resulting resize must land on a subsequent tick, not this one.
      const callback: ResizeObserverMockCallback = jest.fn(() => {
        const setWidth = setWidthRef.current;
        if (setWidth != null) {
          setWidth(200);
        }
      });

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });

      // The state update scheduled from inside the callback is processed as
      // its own follow-up task (a later pass of the resize-observation step,
      // not merged into the same delivery as the commit that triggered it).
      // Setting the same width again is then a no-op for React, so this
      // settles after exactly one follow-up round instead of looping
      // indefinitely.
      expect(callback).toHaveBeenCalledTimes(2);
      expect(callback.mock.calls[0][0][0].contentRect.width).toBe(100);
      expect(callback.mock.calls[1][0][0].contentRect.width).toBe(200);
    });
  });

  describe('unobserve(target)', () => {
    it('should throw if `target` is not a `ReactNativeElement`', () => {
      observer = new ResizeObserver(() => {});
      expect(() => {
        // $FlowExpectedError[incompatible-type]
        observer.unobserve('something');
      }).toThrow(
        "Failed to execute 'unobserve' on 'ResizeObserver': parameter 1 is not of type 'ReactNativeElement'.",
      );
    });

    it('should ignore the call if `target` was not observed (not fail)', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();
      Fantom.runTask(() => {
        root.render(<View style={{width: 10, height: 10}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.unobserve(node);
      });

      expect(callback).not.toHaveBeenCalled();
    });

    it('should stop observing the target if it was observed', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        observer.unobserve(node);
      });

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 200, height: 50}} ref={nodeRef} />,
        );
      });

      expect(callback).toHaveBeenCalledTimes(1);
    });

    it('should stop observing the target if it was observed (detached target after observing)', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        root.render(<></>);
      });
      expect(node.isConnected).toBe(false);
      // Removal from the tree delivers one final 0x0 observation (Web parity).
      expect(callback).toHaveBeenCalledTimes(2);

      Fantom.runTask(() => {
        expect(() => {
          observer.unobserve(node);
        }).not.toThrow();
      });
      // No further deliveries after unobserve.
      expect(callback).toHaveBeenCalledTimes(2);
    });

    it('should not report the initial state if the target is unobserved before it is delivered', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
        observer.unobserve(node);
      });

      expect(callback).not.toHaveBeenCalled();
    });

    it('should not deliver a stale entry when the target is unobserved after being dirtied but before delivery', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 100, height: 50}} ref={nodeRef} />,
        );
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      // The commit inside this task dirties the target's family (recorded
      // as a raw pointer in `dirtyFamiliesBySurfaceId_`), and `unobserve`
      // runs before `runResizeObservations` drains that map at the end of
      // the event-loop tick. This must not crash and must not deliver a
      // second (stale) entry.
      Fantom.runTask(() => {
        root.render(
          <View key="target" style={{width: 200, height: 75}} ref={nodeRef} />,
        );
        observer.unobserve(node);
      });

      expect(callback).toHaveBeenCalledTimes(1);
    });

    it('should work with multiple resize observer instances', () => {
      const nodeRef = createRef<HostInstance>();
      let observer1: ResizeObserver;
      let observer2: ResizeObserver;

      const root = Fantom.createRoot();
      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);

      Fantom.runTask(() => {
        observer1 = new ResizeObserver(() => {});
        observer2 = new ResizeObserver(() => {});

        observer1.observe(node);
        observer2.observe(node);

        observer1.unobserve(node);

        // The second call shouldn't log errors (that would make the test fail).
        observer2.unobserve(node);
      });
    });
  });

  describe('disconnect()', () => {
    it('should do nothing if no targets are observed (not fail)', () => {
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.disconnect();
      });

      expect(callback).not.toHaveBeenCalled();
    });

    it('should stop observing all observed targets', () => {
      const node1Ref = createRef<HostInstance>();
      const node2Ref = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(
          <>
            <View key="node1" style={{width: 50, height: 50}} ref={node1Ref} />
            <View
              key="node2"
              style={{width: 200, height: 200}}
              ref={node2Ref}
            />
          </>,
        );
      });

      const node1 = ensureReactNativeElement(node1Ref.current);
      const node2 = ensureReactNativeElement(node2Ref.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node1);
        observer.observe(node2);
      });
      expect(callback).toHaveBeenCalledTimes(1);

      Fantom.runTask(() => {
        observer.disconnect();
      });

      Fantom.runTask(() => {
        root.render(
          <>
            <View key="node1" style={{width: 60, height: 50}} ref={node1Ref} />
            <View
              key="node2"
              style={{width: 220, height: 200}}
              ref={node2Ref}
            />
          </>,
        );
      });

      expect(callback).toHaveBeenCalledTimes(1);
    });

    it('should not dispatch pending entries when disconnecting', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);

        // At the end of the current tick, we schedule delivery of the initial
        // observation for the target.
        observer.observe(node);

        // This is executed in the next tick, before the resize observer
        // callback is called.
        Fantom.scheduleTask(() => {
          expect(callback).not.toHaveBeenCalled();

          observer.disconnect();

          expect(callback).toHaveBeenCalledTimes(0);
        });
      });
    });

    it('should correctly unobserve targets that are disconnected after observing', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);

      Fantom.runTask(() => {
        observer = new ResizeObserver(() => {});
        observer.observe(node);
      });

      Fantom.runTask(() => {
        root.render(<></>);
      });
      expect(node.isConnected).toBe(false);

      expect(() => {
        observer.disconnect();
      }).not.toThrow();
    });
  });

  describe('ResizeObserverEntry', () => {
    it('should freeze size arrays and keep stable getter identities', () => {
      const nodeRef = createRef<HostInstance>();
      const root = Fantom.createRoot();

      Fantom.runTask(() => {
        root.render(<View style={{width: 100, height: 50}} ref={nodeRef} />);
      });

      const node = ensureReactNativeElement(nodeRef.current);
      const callback = jest.fn();

      Fantom.runTask(() => {
        observer = new ResizeObserver(callback);
        observer.observe(node);
      });

      const [entries] = callback.mock.lastCall;
      const entry = entries[0];

      expect(Object.isFrozen(entry.contentBoxSize)).toBe(true);
      expect(Object.isFrozen(entry.borderBoxSize)).toBe(true);
      expect(Object.isFrozen(entry.devicePixelContentBoxSize)).toBe(true);
      expect(entry.contentBoxSize).toBe(entry.contentBoxSize);
      expect(entry.borderBoxSize).toBe(entry.borderBoxSize);
      expect(entry.devicePixelContentBoxSize).toBe(
        entry.devicePixelContentBoxSize,
      );
      expect(entry.contentRect).toBe(entry.contentRect);
    });
  });

  describe('ResizeObserverEntry global constructor', () => {
    it('throws when called', () => {
      expect(
        () =>
          // The public stub throws regardless of arguments; the real class
          // requires two so Flow needs a suppression here.
          // $FlowExpectedError[incompatible-type]
          new ResizeObserverEntry(),
      ).toThrow(
        "Failed to construct 'ResizeObserverEntry': Illegal constructor",
      );
    });
  });

  describe('ResizeObserverSize global constructor', () => {
    it('throws when called', () => {
      expect(
        () =>
          // The public stub throws regardless of arguments; the real class
          // requires two so Flow needs a suppression here.
          // $FlowExpectedError[incompatible-type]
          new ResizeObserverSize(),
      ).toThrow(
        "Failed to construct 'ResizeObserverSize': Illegal constructor",
      );
    });
  });
});
