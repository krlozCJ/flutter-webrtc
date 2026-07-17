import 'dart:io';

import 'package:flutter_webrtc/src/extensions/mediadevices_ext.dart';
import 'package:flutter_webrtc/src/native/factory_impl.dart';
import 'package:flutter_webrtc/src/native/utils.dart';
import 'package:webrtc_interface/webrtc_interface.dart';

extension ProxyTrack on MediaStreamTrack {
  Future<MediaStreamTrack> generateProxyTrack(String tagName) async {
    if (label != "sinthetic") {
      throw Exception("Cannot generate proxy from proxy");
    }
    final track = await navigator.mediaDevices.createSintheticTrack(tagName);

    return track;
  }

  Future<void> attachToTrack(MediaStreamTrack track) async {
    if (track.id != null && track.id == id) {
      throw Exception("Cannot auto attach");
    }

    await WebRTC.invokeMethod(
        "attachToTrack", {"sintheticTrackId": id, "trackId": track.id});
  }

  Future<void> setFilter() async {
    if (Platform.isWindows && label != "sinthetic") {
      throw Exception("Only Sinthetic tracks can be setted filters");
    }

    if (!Platform.isAndroid) {
      throw UnsupportedError("Filters not supported");
    }

    await WebRTC.invokeMethod("setFilter", {});
  }
}
