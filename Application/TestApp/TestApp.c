/** @file
  Verify MSR 0xE2 status on all the processors.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcProtocolLib.h>
#include <Library/OcAppleBootPolicyLib.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/DevicePathPropertyDatabase.h>


#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleFileSystem.h>


VOID
TestDeviceProperties (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                  Status;
  EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *PropertyDatabase;
  EFI_DEVICE_PATH_PROTOCOL                    *DevicePath;
  CHAR16                                      *TextualDevicePath;
  UINTN                                       ReadSize;

  UINT8                                       Data[4] = {0x11, 0x22, 0x33, 0x44};
  UINT8                                       ReadData[4];

  Print (L"TestDeviceProperties\n");

  Status = UninstallAllProtocolInstances (&gEfiDevicePathPropertyDatabaseProtocolGuid);

  Print (L"UninstallAllProtocolInstances is %r\n", Status);

  Status = OcDevicePathPropertyInstallProtocol (ImageHandle, SystemTable);

  Print (L"OcDevicePathPropertyInstallProtocol is %r\n", Status);

  Status = gBS->LocateProtocol (
    &gEfiDevicePathPropertyDatabaseProtocolGuid,
    NULL,
    (VOID **) &PropertyDatabase
    );

  Print (L"Locate gEfiDevicePathPropertyDatabaseProtocolGuid is %r\n", Status);

  DevicePath = ConvertTextToDevicePath (L"PciRoot(0x0)/Pci(0x11,0x0)/Pci(0x1,0x0)");

  Print (L"Binary PciRoot(0x0)/Pci(0x11,0x0)/Pci(0x1,0x0) is %p\n", DevicePath);

  TextualDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);

  Print (L"Textual is is %s\n", TextualDevicePath ? TextualDevicePath : L"<null>");

  Status = PropertyDatabase->SetProperty (
    PropertyDatabase,
    DevicePath,
    L"to-the-sky",
    Data,
    sizeof (Data)
    );

  Print (L"Set to-the-sky is %r\n", Status);

  ZeroMem (ReadData, sizeof (ReadData));
  ReadSize = sizeof (ReadData);

  Status = PropertyDatabase->GetProperty (
    PropertyDatabase,
    DevicePath,
    L"to-the-sky",
    ReadData,
    &ReadSize
    );
  
  Print (L"Get to-the-sky is %r %u <%02x %02x %02x %02x>\n", Status, (UINT32) ReadSize, ReadData[0], ReadData[1], ReadData[2], ReadData[3]);
}

VOID
TestBless (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                  Status;
  APPLE_BOOT_POLICY_PROTOCOL                  *AppleBootPolicy;
  UINTN                                       Index;
  CHAR16                                      *TextualDevicePath1;
  CHAR16                                      *TextualDevicePath2;
  CHAR16                                      *BootEntryName;
  CHAR16                                      *BootPathName;
  OC_BOOT_ENTRY                               *Entries;
  UINTN                                       EntryCount;
  
  Print (L"TestBless\n");

  Status = UninstallAllProtocolInstances (&gAppleBootPolicyProtocolGuid);

  Print (L"UninstallAllProtocolInstances is %r\n", Status);

  Status = OcAppleBootPolicyInstallProtocol (ImageHandle, SystemTable);

  Print (L"OcAppleBootPolicyInstallProtocol is %r\n", Status);

  Status = gBS->LocateProtocol (
    &gAppleBootPolicyProtocolGuid,
    NULL,
    (VOID **) &AppleBootPolicy
    );

  if (EFI_ERROR (Status)) {
    Print (L"Locate gAppleBootPolicyProtocolGuid failed %r\n", Status);
    return;
  }

  Status = OcScanForBootEntries (
    AppleBootPolicy,
    0,
    &Entries,
    &EntryCount
    );

  if (EFI_ERROR (Status)) {
    Print (L"Locate OcScanForBootEntries failed %r\n", Status);
    return;
  }

  Print (L"Located %u bless entries\n", (UINT32) EntryCount);

  for (Index = 0; Index < EntryCount; ++Index) {
    Status = OcDescribeBootEntry (
      AppleBootPolicy,
      Entries[Index].DevicePath,
      &BootEntryName,
      &BootPathName
      );

    if (EFI_ERROR (Status)) {
      FreePool (Entries[Index].DevicePath);
      continue;
    }

    TextualDevicePath1 = ConvertDevicePathToText (Entries[Index].DevicePath, FALSE, FALSE);
    TextualDevicePath2 = ConvertDevicePathToText (Entries[Index].DevicePath, TRUE, TRUE);

    Print (L"%u. Bless Entry <%s> (on handle %p, dmg %d)\n",
      (UINT32) Index, BootEntryName, Entries[Index], Entries[Index].PrefersDmgBoot);
    Print (L"Full path: %s\n", TextualDevicePath1 ? TextualDevicePath1 : L"<null>");
    Print (L"Short path: %s\n", TextualDevicePath2 ? TextualDevicePath2 : L"<null>");
    Print (L"Dir path: %s\n", BootPathName);

    if (TextualDevicePath1 != NULL) {
      FreePool (TextualDevicePath1);
    }

    if (TextualDevicePath2 != NULL) {
      FreePool (TextualDevicePath2);
    }

    FreePool (Entries[Index].DevicePath);
    FreePool (BootEntryName);
    FreePool (BootPathName);
  }

  FreePool (Entries);
}

EFI_STATUS
FindWritableFs (
  OUT EFI_FILE_PROTOCOL **WritableFs
  )
{
  EFI_HANDLE  *HandleBuffer = NULL;
  UINTN       HandleCount;
  UINTN       Index;
  
  // Locate all the simple file system devices in the system
  EFI_STATUS Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (!EFI_ERROR (Status)) {
    EFI_FILE_PROTOCOL *Fs = NULL;
    // For each located volume
    for (Index = 0; Index < HandleCount; Index++) {
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFs = NULL;
      EFI_FILE_PROTOCOL *File = NULL;
      
      // Get protocol pointer for current volume
      Status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **) &SimpleFs);
      if (EFI_ERROR (Status)) {
        DEBUG((-1, "FindWritableFs: gBS->HandleProtocol[%d] returned %r\n", Index, Status));
        continue;
      }
      
      // Open the volume
      Status = SimpleFs->OpenVolume(SimpleFs, &Fs);
      if (EFI_ERROR (Status)) {
        DEBUG((-1, "FindWritableFs: SimpleFs->OpenVolume[%d] returned %r\n", Index, Status));
        continue;
      }
      
      // Try opening a file for writing
      Status = Fs->Open(Fs, &File, L"crsdtest.fil", EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
      if (EFI_ERROR (Status)) {
        DEBUG((-1, "FindWritableFs: Fs->Open[%d] returned %r\n", Index, Status));
        continue;
      }
      
      // Writable FS found
      Fs->Delete(File);
      *WritableFs = Fs;
      Status = EFI_SUCCESS;
      break;
    }
  }
  
  // Free memory
  if (HandleBuffer) {
      gBS->FreePool(HandleBuffer);
  }
  
  return Status;
}

EFI_STATUS
WriteFirmware (
  VOID
  )
{
  EFI_STATUS        Status;
  EFI_FILE_PROTOCOL *Fs = NULL;
  EFI_FILE_PROTOCOL *File = NULL;
  VOID              *Address;
  UINTN             AreaSize;

  // Find writable FS
  Status = FindWritableFs (&Fs);
  if (EFI_ERROR (Status)) {
    DEBUG((-1, "WriteFirmware: Can't find writable FS\n"));
    return Status;
  }

  // Open or create output file
  Status = Fs->Open(Fs, &File, L"uefi-firmware.bin", EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (EFI_ERROR (Status)) {
    DEBUG((-1, "WriteFirmware: Fs->Open of %s returned %r\n", L"uefi-firmware.bin", Status));
    return Status;
  }

  // Write FW image into the file and close it
  AreaSize = 0x1000000ULL;
  Address = (VOID *)0xFF000000ULL;
  Status = File->Write(File, &AreaSize, Address);
  File->Close(File);
  if (EFI_ERROR (Status)) {
    DEBUG((-1, "WriteFirmware: File->Write returned %r\n", Status));
    return Status;
  }

  return Status;
}


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN  Index;

  WaitForKeyPress (L"Press any key...");

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    Print (L"Table %u is %g\n", (UINT32) Index, gST->ConfigurationTable[Index].VendorGuid);
  }

  Print (L"This is test app...\n");

  // TestDeviceProperties (ImageHandle, SystemTable);

  TestBless (ImageHandle, SystemTable);

  // WriteFirmware ();

  return EFI_SUCCESS;
}
