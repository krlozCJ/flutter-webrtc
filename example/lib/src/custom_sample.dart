import 'package:flutter/material.dart';
import 'package:flutter_webrtc/flutter_webrtc.dart';

class CustomSample extends StatefulWidget {
  @override
  State<CustomSample> createState() => _CustomSampleState();
}

class _CustomSampleState extends State<CustomSample> {
  RTCVideoRenderer? renderer;

  MediaStream? mediaStream;
  MediaStreamTrack? origin;
  MediaStreamTrack? trackProxy;

  bool rendering = false;

  @override
  void initState() {
    super.initState();

    renderer = RTCVideoRenderer();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Prueba de proxy'),
      ),
      body: Column(
        children: [
          Expanded(
              child: Container(
            color: Colors.black,
            child: rendering
                ? RTCVideoView(
                    renderer!,
                    objectFit:
                        RTCVideoViewObjectFit.RTCVideoViewObjectFitContain,
                    placeholderBuilder: (context) => Container(
                      color: Colors.red,
                    ),
                  )
                : null,
          )),
          SizedBox(
            height: 10,
          ),
          Row(
            children: [
              SizedBox(
                width: 10,
              ),
              ElevatedButton(
                  onPressed: _startRenderer, child: Text('Iniciar renderer')),
              SizedBox(
                width: 10,
              ),
              ElevatedButton(
                  onPressed: callCamera, child: Text('Iniciar cámara')),
              SizedBox(
                width: 10,
              ),
              ElevatedButton(
                  onPressed: _startRender, child: Text('Iniciar render')),
              SizedBox(
                width: 10,
              ),
              ElevatedButton(
                  onPressed: _switching, child: Text('Cambiar renders')),
              SizedBox(
                width: 10,
              ),
            ],
          ),
          SizedBox(
            height: 10,
          ),
        ],
      ),
    );
  }

  @override
  void dispose() async {
    super.dispose();
    // await mediaStream?.removeTrack(trackProxy!);
    trackProxy = null;
    // await trackProxy?.stop();
    await mediaStream?.dispose();
    await origin?.stop();
    await renderer!.dispose();
    print("Si se ejecutó esto?");
  }

  void _startRenderer() async {
    await renderer!.initialize();
  }

  void _startRender() async {
    try {
      renderer!.srcObject = mediaStream;
      setState(() {
        rendering = true;
      });
    } catch (e) {
      print("No se pudo iniciar el render: $e");
    }
  }

  void _switching() async {
    await mediaStream!.removeTrack(trackProxy!);
    await mediaStream!.addTrack(origin!);
    await renderer!.setSrcObject(stream: mediaStream);
  }

  void callCamera() async {
    try {
      final deviceId =
          (await navigator.mediaDevices.enumerateDevices()).firstWhere((d) {
        print(d.label);
        return d.label.contains("HD Pro");
      }).deviceId;
      // final deviceId =
      //     (await navigator.mediaDevices.enumerateDevices()).first.deviceId;

      final constrain = {
        'video': deviceId == null
            ? true
            : {
                'mandatory': {'minFrameRate': 60},
                'optional': [
                  {'sourceId': deviceId},
                ],
              },
        'audio': true,
      };

      mediaStream = await navigator.mediaDevices.getUserMedia(constrain);

      origin = mediaStream!.getVideoTracks().first;
      trackProxy = await origin!.createProxy();

      print('Tracks: [${origin!.id} / ${trackProxy!.id}]');

      await mediaStream!.removeTrack(origin!);
      await mediaStream!.addTrack(trackProxy!);
      print(mediaStream!.getVideoTracks().first.id);
    } catch (e, s) {
      print('imposible $e');
      print(s);
      try {
        await origin?.stop();
      } catch (e) {
        print('No se pudo liberar el origen: $e');
      }
      try {
        await trackProxy?.stop();
      } catch (e) {
        print('No se pudo liberar el proxy: $e');
      }
      try {
        await mediaStream?.dispose();
      } catch (e) {
        print('No se pudo liberar el stream: $e');
      }
    }
  }
}
