//
//  Module.swift
//  Flipper
//
//  Created by Harlan Haskins on 2/9/18.
//

import Foundation
import Clibflipper

public struct StandardModuleFFI {
  let moduleMetadata: _lf_module
}

public struct UserModuleFFI {
  var moduleMetadata: _lf_module
  let name: String

  public static func uninitialized(name: String) -> UserModuleFFI {
    return UserModuleFFI(name: name, version: 0, crc: 0, index: 0)
  } 

  init(name: String, version: lf_version_t, crc: lf_crc_t, index: Int32) {
    self.name = name
    self.moduleMetadata = _lf_module(name: name,
                                     description: nil,
                                     version: version,
                                     identifier: crc,
                                     index: index,
                                     device: nil,
                                     data: nil,
                                     psize: nil)
  }
}

func buildLinkedList(_ args: [LFArg]) -> UnsafeMutablePointer<_lf_ll>? {
  var argList: UnsafeMutablePointer<_lf_ll>? = nil
  for arg in args {
    lf_ll_append(&argList,
                 UnsafeMutableRawPointer(bitPattern:
                  UInt(truncatingIfNeeded: arg.asLFArg.value)), nil)
  }
  return argList
}

public enum ModuleFFI {
  case standard(StandardModuleFFI)
  case user(UserModuleFFI)

  var modulePtr: _lf_module {
    switch self {
    case .standard(let mod): return mod.moduleMetadata
    case .user(let mod): return mod.moduleMetadata
    }
  }

  public func invoke(index: UInt8, args: [LFArg]) {
    _ = invoke(index: index, args: args) as LFVoid
  }

  public func push(index: UInt8, data: Data, args: [LFArg]) {
    _ = push(index: index, data: data, args: args) as LFVoid
  }

  public func pull(index: UInt8, data: inout Data, args: [LFArg]) {
    _ = pull(index: index, data: &data, args: args) as LFVoid
  }

  public func invoke<Ret: LFReturnable>(index: UInt8, args: [LFArg]) -> Ret {
    var mod = modulePtr
    let ret = lf_invoke(&mod, index, Ret.lfType.rawValue, buildLinkedList(args))
    return Ret.init(lfReturn: ret)
  }

  public func push<Ret: LFReturnable>(index: UInt8, data: Data, args: [LFArg]) -> Ret {
    var mod = modulePtr
    let ret = 
      data.withUnsafeBytes { (ptr: UnsafePointer<UInt32>) -> lf_return_t in
        return lf_push(&mod, index, UnsafeMutableRawPointer(mutating: ptr),
                       UInt32(data.count), buildLinkedList(args))
      }
    return Ret.init(lfReturn: ret)
  }

  public func pull<Ret: LFReturnable>(index: UInt8, data: inout Data,
                                      args: [LFArg]) -> Ret {
    var mod = modulePtr
    let count = UInt32(data.count)
    let ret = data.withUnsafeMutableBytes {
      lf_pull(&mod, index, $0, count, buildLinkedList(args))
    }
    return Ret.init(lfReturn: ret)
  }
}

public protocol UserModule {
  static var name: String { get }
  init(flipper: Flipper)
  init(ffi: ModuleFFI)
  init()
}

public extension UserModule {
  init() {
    self.init(ffi: .user(.uninitialized(name: Self.name)))
  }

  init(flipper: Flipper) {
    var ffi = UserModuleFFI(name: Self.name, version: 0, crc: 0, index: 0)
    lf_bind(&ffi.moduleMetadata, flipper.device)
    self.init(ffi: .user(ffi))
  }
}

public protocol StandardModule {
  static var underlyingModule: _lf_module { get }
  init(flipper: Flipper)
  init(ffi: ModuleFFI)
  init()
}

public extension StandardModule {
  init() {
    self.init(ffi: .standard(StandardModuleFFI(moduleMetadata: Self.underlyingModule)))
  }

  init(flipper: Flipper) {
    var mod = Self.underlyingModule
    lf_bind(&mod, flipper.device)
    self.init(ffi: .standard(StandardModuleFFI(moduleMetadata: mod)))
  }
}