using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Prisma.Core
{
    public static class SaveGame
    {
        public static unsafe bool Save(string slotName, string jsonData)
        {
            byte[] slotBytes = Encoding.UTF8.GetBytes(slotName + "\0");
            byte[] dataBytes = Encoding.UTF8.GetBytes(jsonData + "\0");
            fixed (byte* pSlot = slotBytes)
            fixed (byte* pData = dataBytes)
            {
                return Interop.API.SaveGameSave(pSlot, pData);
            }
        }

        public static unsafe string Load(string slotName)
        {
            byte[] slotBytes = Encoding.UTF8.GetBytes(slotName + "\0");
            byte* result;
            fixed (byte* pSlot = slotBytes)
            {
                result = Interop.API.SaveGameLoad(pSlot);
            }
            if (result == null) return null;
            string str = Marshal.PtrToStringUTF8((IntPtr)result);
            Interop.API.FreeAssetData(result);
            return str;
        }

        public static unsafe bool Delete(string slotName)
        {
            byte[] slotBytes = Encoding.UTF8.GetBytes(slotName + "\0");
            fixed (byte* pSlot = slotBytes)
            {
                return Interop.API.SaveGameDelete(pSlot);
            }
        }

        public static unsafe string[] ListSlots()
        {
            byte* result = Interop.API.SaveGameListSlots();
            if (result == null) return new string[0];
            string json = Marshal.PtrToStringUTF8((IntPtr)result);
            Interop.API.FreeAssetData(result);
            json = json.TrimStart('[').TrimEnd(']');
            if (string.IsNullOrEmpty(json)) return new string[0];
            string[] parts = json.Split(',');
            string[] slots = new string[parts.Length];
            for (int i = 0; i < parts.Length; i++)
            {
                slots[i] = parts[i].Trim().Trim('"');
            }
            return slots;
        }
    }
}
