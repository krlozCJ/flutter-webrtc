import 'package:webrtc_interface/webrtc_interface.dart';

import '../media_stream_track_impl.dart';

class SintheticTrack extends MediaStreamTrackNative{
  SintheticTrack(super.trackId, super.label, super.kind, super.enabled, super.peerConnectionId);

  Future<void> attachToTrack(MediaStreamTrack track) async {
    
  }
}