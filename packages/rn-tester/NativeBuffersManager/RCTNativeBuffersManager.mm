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
#import <objc/runtime.h>

@interface LoggingMutableData : NSMutableData
@end

@implementation LoggingMutableData

+ (instancetype)dataWithData:(NSData *)data {
  LoggingMutableData *instance = (LoggingMutableData *)[[NSMutableData alloc] initWithData:data];
  object_setClass(instance, [LoggingMutableData class]);
  return instance;
}

- (void)dealloc {
  NSLog(@"LoggingMutableData deallocated");
}

@end

static NSUInteger getBase64Length(NSString *string) {
  NSData *data = [[NSData alloc]
    initWithBase64EncodedString:string options:0];
  return (NSUInteger)[data length];
}

@implementation RCTNativeBuffersManager {
  facebook::react::ModuleConstants<JS::NativeBuffersManager::Constants> _constants;
}

+ (NSString *)moduleName { 
  return @"BuffersManager";
}

- (void)initialize
{
  _constants = facebook::react::typedConstants<JS::NativeBuffersManager::Constants>({
    .text = @"heavy-data",
    .buffer = [NSMutableData dataWithData:[@"heavy-data" dataUsingEncoding:NSUTF8StringEncoding]]
  });
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


- (nonnull NSMutableData *)getBuffer {
  return [_constants objectForKey:@"buffer"];
}


- (void)processBase64:(nonnull NSString *)buffer {
  NSUInteger length = getBase64Length(buffer);
  NSLog(@"Base64 buffer length: %lu", length);
  [self emitOnMyString:@"strong"];
}

- (nonnull NSMutableData *)getBuffer:(NSInteger)size { 
  NSMutableData *buffer = [NSMutableData dataWithLength:size];
    uint8_t *bytes = (uint8_t *)[buffer mutableBytes];
    for (NSInteger i = 0; i < size; ++i) {
      bytes[i] = arc4random_uniform(256);
    }
    return buffer;
}


- (nonnull NSString *)processUnion:(nonnull NSDictionary *)object { 
  if ([object objectForKey:@"buffer"] != nil) {
    NSData *bufferData = [object objectForKey:@"buffer"];
    NSLog(@"Union buffer length: %lu", (unsigned long)[bufferData length]);
    return @"buffer";
  } else if ([object objectForKey:@"text"] != nil) {
    NSString *textData = [object objectForKey:@"text"];
    NSUInteger length = getBase64Length(textData);
    NSLog(@"Union text length: %lu", length);
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
    NSUInteger length = getBase64Length(object.text());
    NSLog(@"Struct text length: %lu", length);
  }
  return NULL;
}

- (nonnull facebook::react::ModuleConstants<JS::NativeBuffersManager::Constants>)constantsToExport {
  return (facebook::react::ModuleConstants<JS::NativeBuffersManager::Constants>)[self getConstants];
}


- (nonnull facebook::react::ModuleConstants<JS::NativeBuffersManager::Constants>)getConstants {
  return _constants;
}

- (NSMutableData * _Nullable)processArrayOfBuffers:(nonnull NSArray *)buffers {
  if ([buffers count] > 0) {
    NSMutableData* buffer = [buffers objectAtIndex:0];
    NSLog(@"Buffer from Array length: %lu", (unsigned long)[buffer length]);
    return buffer;
 }
  return NULL;
}

- (void)processUnsafe:(nonnull NSDictionary *)buffer { 
  NSMutableData *mutableBuffer = (NSMutableData *)buffer;
  NSUInteger length = [mutableBuffer length] / sizeof(uint8_t);
  NSLog(@"UnsafeBuffer length: %lu", (unsigned long)length);

}

- (void)getAsyncBuffer:(nonnull RCTPromiseResolveBlock)resolve reject:(nonnull RCTPromiseRejectBlock)reject {
  auto size = 8;
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
    NSMutableData *buffer = [NSMutableData dataWithLength:size];
    uint8_t *bytes = (uint8_t *)[buffer mutableBytes];
    for (NSInteger i = 0; i < size; ++i) {
      bytes[i] = arc4random_uniform(256);
    }
    resolve(buffer);
  });
}

- (void)processOptionalBuffer:(NSMutableData * _Nullable)buffer { 
  if (buffer != nil) {
    NSLog(@"Optional buffer length: %lu", (unsigned long)[buffer length]);
  } else {
    NSLog(@"Optional buffer is null");
  }
}


@end
