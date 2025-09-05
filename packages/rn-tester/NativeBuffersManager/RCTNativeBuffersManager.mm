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

  [self emitOnMyBuffer:buffer];
}

- (void)processBase64:(nonnull NSString *)buffer {
  NSData *data = [[NSData alloc]
    initWithBase64EncodedString:buffer options:0];
  NSUInteger length = (unsigned long)[data length];
  NSLog(@"Base64 buffer length: %lu", length);
  
  [self emitOnMyString:@"strong"];
}

- (nonnull NSString *)processUnion:(nonnull NSDictionary *)object { 
  if ([object objectForKey:@"buffer"] != nil) {
    NSData *bufferData = [object objectForKey:@"buffer"];
    NSLog(@"Union buffer length: %lu", (unsigned long)[bufferData length]);
    return @"buffer";
  } else if ([object objectForKey:@"text"] != nil) {
    NSString *textData = [object objectForKey:@"text"];
    NSLog(@"Union text length: %lu", (unsigned long)[textData length]);
    return @"text";
  } else {
    return @"other";
  }
}

- (NSMutableData * _Nullable)processArrayBufferStruct:(JS::NativeBuffersManager::ArrayBufferStruct &)object { 
  if (object.buffer()) {
    NSLog(@"Struct buffer length: %lu", (unsigned long)[object.buffer() length]);
  }
  return NULL;
}


- (NSString * _Nullable)processStringStruct:(JS::NativeBuffersManager::StringStruct &)object { 
  if (object.text()) {
    NSLog(@"Struct text length: %lu", (unsigned long)[object.text() length]);
  }
  return NULL;
}



@end
