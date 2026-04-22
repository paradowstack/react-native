/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#import <UIKit/UIKit.h>

#import <React/RCTPrimitives.h>
#import <react/renderer/core/LayoutContext.h>
#import <react/renderer/runtimescheduler/RuntimeScheduler.h>

NS_ASSUME_NONNULL_BEGIN

@class RCTMountingManager;

/**
 * MountingManager's delegate.
 */
@protocol RCTMountingManagerDelegate <NSObject>

/*
 * Called right *before* execution of mount items which affect a Surface with
 * given `rootTag`.
 * Always called on the main queue.
 */
- (void)mountingManager:(RCTMountingManager *)mountingManager willMountComponentsWithRootTag:(ReactTag)rootTag;

/*
 * Called right *after* execution of mount items which affect a Surface with
 * given `rootTag`.
 * Always called on the main queue.
 */
- (void)mountingManager:(RCTMountingManager *)mountingManager didMountComponentsWithRootTag:(ReactTag)rootTag;

/*
 * Returns the current `LayoutContext` for the surface identified by `rootTag`
 * (same as `SurfaceId`). Used when applying layout-dependent style resolution
 * on the main thread (e.g. `calc()` with viewport units).
 * Always called on the main queue.
 */
- (facebook::react::LayoutContext)mountingManager:(RCTMountingManager *)mountingManager
                        layoutContextForRootTag:(facebook::react::SurfaceId)rootTag;

@end

NS_ASSUME_NONNULL_END
