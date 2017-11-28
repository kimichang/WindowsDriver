#include "HelloWDM.h"

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	KdPrint(("Enter DriverEntry\n"));

	pDriverObject->DriverExtension->AddDevice = HelloWDMAddDevice;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = HelloWDMPnp;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
		pDriverObject->MajorFunction[IRP_MJ_CREATE] =
		pDriverObject->MajorFunction[IRP_MJ_READ] =
		pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloWDMDispatchRoutine;
	pDriverObject->DriverUnload = HelloWDMUnload;

	KdPrint(("Leave DriverEntry\n"));

	return STATUS_SUCCESS;
}

#pragma PAGECODE
NTSTATUS HelloWDMAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
{
	PAGED_CODE();
	KdPrint(("Enter HelloWDMAddDevice\n"));

	NTSTATUS status;
	PDEVICE_OBJECT fdo;
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, L"\\Device||MyWDMDevice");
	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &(UNICODE_STRING)devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &fdo);

	if (!NT_SUCCESS(status))
		return status;

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	pdx->fdo = fdo;
	pdx->NextStatckDevice = IoAttachDeviceToDeviceStack(fdo, PhysicalDeviceObject);
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, L"\\DosDevices\\HwlloWDM");
	pdx->ustrDeviceName = devName;
	pdx->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&(UNICODE_STRING)symLinkName,&(UNICODE_STRING)devName);

	if (!NT_SUCCESS(status))
	{
		IoDeleteSymbolicLink(&pdx->ustrSymLinkName);
		status = IoCreateSymbolicLink(&symLinkName, &devName);
		if (!NT_SUCCESS(status))
		{
			return status;
		}
	}

	fdo->Flags != DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= -DO_DEVICE_INITIALIZING;
	KdPrint(("Leaving HelloWDMAddDevice\n"));
	return STATUS_SUCCESS;


}


#pragma PAGECODE
NTSTATUS HelloWDMPnp(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PAGED_CODE();
	KdPrint(("Entering HelloWDMPnp\n"));
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	static NTSTATUS(*fcntab[])(PDEVICE_EXTENSION pdx, PIRP Irp) = {
		DefaultPnpHandler,
		DefaultPnpHandler,
		HandleRemoveDevice,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
		DefaultPnpHandler,
	};
	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
	{
		status = DefaultPnpHandler(pdx, Irp);
		return status;
	}

#if DBG
	static char* fcnname[] = {
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOCE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCEL_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_QUERY_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_QUERY_PNP_DEVICE_STATE",
		"IRP_MN_QUERY_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL",



	};
	KdPrint(("Pnp Request %s\n", fcnname[fcn]));
#endif

	status = (*fcntab[fcn])(pdx, Irp);
	KdPrint(("Leaving HelloWDMPnp\n"));
	return status;
}


#pragma PAGEDCODE
NTSTATUS DefaultPnpHandler(PDEVICE_EXTENSION pdx, PIRP Irp)
{
	PAGED_CODE();
	KdPrint(("Enter DefaultPnpHandler\n"));
	IoSkipCurrentIrpStackLocation(Irp);
	KdPrint(("Leave DefaultPnpHandler\n"));
	return IoCallDriver(pdx->NextStatckDevice,Irp);
}


#pragma PAGADCODE
NTSTATUS HandlerRemoveDevice(PDEVICE_EXTENSION pdx, PIRP Irp)
{
	PAGED_CODE();
	KdPrint(("Enter HandleRemoveDevice\n"));
	Irp->IoStatus.Status = STATUS_SUCCESS;
	NTSTATUS status = DefaultPnpHandler(pdx, Irp);
	IoDeleteSymbolicLink(&(UNICODE_STRING)pdx->ustrSymLinkName);

	if (pdx->NextStatckDevice)
		IoDetachDevice(pdx->NextStatckDevice);

	IoDeleteDevice(pdx->fdo);
	KdPrint(("Leave handleRemoveDevice\n"));
	return status;
}

#pragma PAGEDCODE
NTSTATUS helloWDMDispatchRoutine(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PAGED_CODE();
	KdPrint(("Enter HelloWDMDispatchRoutine\n"));
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	KdPrint(("Leaving HelloWDMDispatchRoutine\n"));
	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
void HelloWDMUnload(IN PDRIVER_OBJECT DriverObject)
{
	PAGED_CODE();
	KdPrint(("Enter HelloWDMUnload\n"));
	KdPrint(("Leaving HelloWDMUnload\n"));
}