import 'dart:js_interop';

import '/src/web/factory_impl.dart';
import 'package:web/web.dart' as web;
// import 'package:webrtc_interface/webrtc_interface.dart';

final Expando<web.MediaStreamTrackProcessor> _processor = Expando();
final Expando<web.TransformStream> _transformer = Expando();
final Expando<MediaStreamTrackGenerator> _generator = Expando();

// Declaramos la clase faltante conectándola a la API nativa del navegador
@JS('MediaStreamTrackGenerator')
extension type MediaStreamTrackGenerator._(JSObject _) implements web.MediaStreamTrack {
  external factory MediaStreamTrackGenerator(JSObject init);
  external web.WritableStream get writable;
}

Future<MediaStreamTrack> createProxyTrackImpl(MediaStreamTrack original) async {
  final jsOriginal = (original as MediaStreamTrackWeb).jsTrack;

  final processor = web.MediaStreamTrackProcessor(web.MediaStreamTrackProcessorInit(track: jsOriginal));
  final generator = MediaStreamTrackGenerator({"kind": 'video'}.jsify() as JSObject);
  final transformer = web.TransformStream();

  processor.readable.pipeThrough(transformer as web.ReadableWritablePair).pipeTo(generator.writable);

  final proxyTrack = MediaStreamTrackWeb(generator);

  _processor[proxyTrack] = processor;
  _transformer[proxyTrack] = transformer;
  _generator[proxyTrack] = generator;

  return proxyTrack;
}
void disposeProxyImpl(MediaStreamTrack proxyTrack) {
    // Liberar recursos de JS
    _processor[proxyTrack]?.readable.cancel();
    _generator[proxyTrack]?.stop();
}

void applyFilterImpl(MediaStreamTrack proxyTrack, String filter) {
  print("Implementando filtros");
}