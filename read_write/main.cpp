#include "dispatches.hpp"

NTSTATUS DriverInit() { return STATUS_SUCCESS; }

NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING hooked_driver = RTL_CONSTANT_STRING(L"\\Driver\\Beep");
	PDRIVER_OBJECT hooked_object = nullptr;

	win::ObReferenceObjectByName(&hooked_driver, 0, 0, 0, *win::IoDriverObjectType, KernelMode, nullptr, reinterpret_cast<void**>(&hooked_object));

	if (!hooked_object) {
		print("[uc_driver.sys] failed to get %wZ driver object\n", &hooked_driver);
		return STATUS_UNSUCCESSFUL;
	}

	memory::init();

	print("[uc_driver.sys] %wZ driver object: 0x%p\n", &hooked_driver, hooked_object);

	original_irp = hooked_object->MajorFunction[IRP_MJ_DEVICE_CONTROL];
	
	print("[uc_driver.sys] swapping irp 0x%p with 0x%p\n", original_irp, control);

	hooked_object->DriverStart = driver_object->DriverStart;
	hooked_object->DriverSize = driver_object->DriverSize;	
	hooked_object->DriverInit = reinterpret_cast<PDRIVER_INITIALIZE>(DriverInit);
	hooked_object->DriverStartIo = nullptr;
	hooked_object->DriverUnload = nullptr;
	/*
	https://www.unknowncheats.me/forum/anti-cheat-bypass/495701-ud-eac-spoof-legit-module.html
	modify (anti paste some left out):
	_KLDR_DATA_TABLE_ENTRY.DllBase
	_DRIVER_OBJECT.DriverStart
	_KLDR_DATA_TABLE_ENTRY.SizeOfImage
	_KLDR_DATA_TABLE_ENTRY.SizeOfImageNotRounded
	_DRIVER_OBJECT.DriverSize
	*/
	hooked_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = control;

	if (!clean::cache())
		print("[uc_driver.sys] failed to clear the PiDDBCacheTable entry\n");

	if (!clean::unloaded_drivers())
		print("[uc_driver.sys] failed to clear the MmUnloadedDrivers list\n");

	print("[uc_driver.sys] done cleaning module traces!\n");

	return STATUS_SUCCESS;
}
