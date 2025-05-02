import asyncio
from bleak import BleakClient, BleakScanner

CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef1"

async def send_loop(client):
    while True:
        user_input = input("Inserisci un numero (o 'esci' ): ")
        if user_input.lower() == "esci":
            break
        await client.write_gatt_char(CHARACTERISTIC_UUID, user_input.encode())
        print("Numero inviato:", user_input)

async def main():
    print("Cerco dispositivi BLE...")
    devices = await BleakScanner.discover()
    address = None
    for d in devices:
        if d.name == "Nano33BLE":
            address = d.address
            break

    if address is None:
        print("Dispositivo non trovato.")
        return

    async with BleakClient(address) as client:
        if client.is_connected:
            print("Connesso a:", address)
            await send_loop(client)

asyncio.run(main())
