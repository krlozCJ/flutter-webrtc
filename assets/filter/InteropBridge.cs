using src;
using System.Runtime.InteropServices;

namespace FilterManager
{
    public static class InteropBridge
    {
        // C++ llama a esto una sola vez al iniciar la cámara
        [UnmanagedCallersOnly(EntryPoint = "RegisterManager")]
        public static IntPtr RegisterManager()
        {
            var manager = new FilterManagerOrchestor();

            // Creamos un Handle para el objeto. Esto le dice al GC de C#: 
            // "No borres este objeto, C++ lo está usando".
            GCHandle handle = GCHandle.Alloc(manager, GCHandleType.Normal);

            // Retornamos el puntero numérico a C++
            return GCHandle.ToIntPtr(handle);
        }

        [UnmanagedCallersOnly(EntryPoint = "ApplyFilter")]
        public static void ApplyFilter(IntPtr managerPtr, IntPtr jsonOptionsPtr)
        {
            if (managerPtr == IntPtr.Zero) return;

            string jsonOptions = Marshal.PtrToStringUTF8(jsonOptionsPtr)!;

            // Recuperamos la instancia directamente usando el puntero
            GCHandle handle = GCHandle.FromIntPtr(managerPtr);
            if (handle.Target is FilterManagerOrchestor manager)
            {
                manager.ApplyOptions(jsonOptions);
            }
        }

        // EL HOT-PATH: Esta función corre 30-60 veces por segundo. 
        // Ahora es rapidísima: cero strings, cero diccionarios.
        [UnmanagedCallersOnly(EntryPoint = "ProcessFrame")]
        public static unsafe void ProcessFrame(IntPtr managerPtr, byte* bytes, int width, int height)
        {
            if (managerPtr == IntPtr.Zero) return;

            try
            {
                GCHandle handle = GCHandle.FromIntPtr(managerPtr);
                if (handle.Target is FilterManagerOrchestor manager)
                {
                    manager.ProcessFrame((IntPtr)bytes, width, height);
                }
            }
            catch
            {
                // Ocultar excepciones para evitar crashes nativos
            }
        }

        // C++ DEBE llamar a esto cuando se apaga la cámara o se destruye el track
        [UnmanagedCallersOnly(EntryPoint = "RemoveManager")]
        public static void RemoveManager(IntPtr managerPtr)
        {
            if (managerPtr != IntPtr.Zero)
            {
                GCHandle handle = GCHandle.FromIntPtr(managerPtr);
                if (handle.Target is FilterManagerOrchestor manager)
                {
                    manager.Dispose();
                }
                // ¡CRÍTICO! Liberar el handle permite que el GC de C# limpie la memoria.
                handle.Free();
            }
        }
    }
}
