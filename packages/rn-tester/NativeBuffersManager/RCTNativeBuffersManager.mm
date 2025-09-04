//
//  RCTNativeBuffersManager.m
//  RNTester
//
//  Created by Kamil Paradowski on 26/08/2025.
//  Copyright © 2025 Facebook. All rights reserved.
//

#import "RCTNativeBuffersManager.h"
#include <random>
#include <memory>
#include <cstring>

@implementation RCTNativeBuffersManager

+ (NSString *)moduleName { 
  return @"BuffersManager";
}

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:(const facebook::react::ObjCTurboModule::InitParams &)params { 
  return std::make_shared<facebook::react::NativeBuffersManagerSpecJSI>(params);
}






- (void)processBuffer:(nonnull NSMutableData *)buffer {
  using type = uint8_t;
  NSUInteger length = [buffer length] / sizeof(type);
  NSLog(@"Buffer length: %lu", (unsigned long)length);
}

- (void)processBase64:(nonnull NSString *)buffer {
  NSData *data = [[NSData alloc] 
    initWithBase64EncodedString:buffer options:0];
  NSUInteger length = (unsigned long)[data length];
  NSLog(@"Base64 buffer length: %lu", length);
}







@end
