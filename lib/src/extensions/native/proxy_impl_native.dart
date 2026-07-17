import 'dart:io';

import 'package:flutter_webrtc/src/native/utils.dart';
import 'package:webrtc_interface/webrtc_interface.dart';

import '../../native/media_stream_track_impl.dart';

Future<MediaStreamTrack> createProxyTrackImpl(MediaStreamTrack original) async {
  if (Platform.isAndroid) return original;
  if (!Platform.isWindows) throw UnimplementedError();

  final response = await WebRTC.invokeMethod(
      "createSintheticTrack", <String, dynamic>{"tagName": original.label});

  final track = response["track"];

  final proxyTrack = MediaStreamTrackNative(track['id'], track['label'],
      track['kind'], track['enabled'], "local", track['settings'] ?? {});

  final responsee = await WebRTC.invokeMethod(
      "attachToTrack", {"sintheticTrackId": proxyTrack.id, "trackId": original.id});

  print("Hubo respuesta positiva $responsee");

  return proxyTrack;
}


void applyFilterImpl(MediaStreamTrack proxyTrack, String filter) {
  print("Implementando filtros");
}

void disposeProxyImpl(MediaStreamTrack proxyTrack) {
  print("Falso dispose");
}