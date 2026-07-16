import 'package:flutter/services.dart';
import 'package:flutter_webrtc/src/native/mediadevices_impl.dart';
import 'package:webrtc_interface/webrtc_interface.dart';

import '../media_stream_track_impl.dart';
import '../utils.dart';

extension MediaDeviceNativeExtension on MediaDeviceNative {
  Future<MediaStreamTrack> createSintheticTrack(String tagName) async {
    try {
      final response = await WebRTC.invokeMethod(
        "createSintheticTrack",
        <String, dynamic>{"tagName": tagName}
      );

      final track = response["track"];

      return MediaStreamTrackNative(
        track['id'], track['label'],
          track['kind'], track['enabled'], "local", track['settings'] ?? {}
      );
    }  on PlatformException catch (e) {
      throw 'Unable to createSintheticTrack: ${e.message}';
    }
  }
}