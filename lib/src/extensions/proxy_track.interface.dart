// Archivo: track_extension.dart
import 'package:flutter_webrtc/flutter_webrtc.dart';
import 'track_proxy_state.dart';

// Magia de Dart: Importa el archivo correcto según la plataforma donde compile
import 'proxy.dart'
    if (dart.library.js_interop) '/src/extensions/native/proxy_impl_web.dart'
    if (dart.library.io) '/src/extensions/native/proxy_impl_native.dart';

extension ProxyTrackExtension on MediaStreamTrack {
  bool get isProxy => isProxyExpando[this] ?? false;
  List<String> get currentFilters => filtersExpando[this] ?? [];

  /// Genera y retorna el track sintético (o el mismo en Android).
  Future<MediaStreamTrack> createProxy() async {
    // if (isProxy) return this; // Evitar proxy de un proxy
    if(isProxy) throw UnsupportedError("Cannot create proxy from another proxy");

    // Llama a la implementación específica de la plataforma
    final proxyTrack = await createProxyTrackImpl(this);

    // Asigna el estado unificado
    isProxyExpando[proxyTrack] = true;
    filtersExpando[proxyTrack] = [];

    return proxyTrack;
  }

  void addFilter(String filter) {
    if (!isProxy) throw Exception("Debes llamar a createProxy() primero");

    final filters = currentFilters;
    filters.add(filter);
    filtersExpando[this] = filters;

    applyFilterImpl(this, filter); // Llama a web o nativo
  }

  void disposeProxy() {
    if (!isProxy) return;

    disposeProxyImpl(this); // Limpia recursos nativos o JS

    // Limpiar estado
    filtersExpando[this] = [];
    isProxyExpando[this] = false;
  }
}
