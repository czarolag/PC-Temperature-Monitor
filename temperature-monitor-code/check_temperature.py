# run this code to test if the port is sending temperature information
# change host to PC local IP that will be sending the updates

import socket
import json

def get_core_temp(host='[LOCAL_IP|Change_This]', port=5200):
    try:
        # Create a socket object
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)  # Timeout if connection takes too long

        # Connect to the server
        s.connect((host, port))
        
        # Request data (Core Temp sends a response to the client upon connection)
        s.sendall(b'')  # Send an empty byte string to trigger the server's response
        
        # Receive the response (assuming a fixed buffer size)
        data = s.recv(1024)

        # Close the socket connection
        s.close()

        # Decode and display the temperature data
        if data:
            data_str = data.decode('utf-8')
            print("Core Temp Data Received:", data_str[:150])  # Print part of the response for inspection
            
            # Parse the JSON data
            try:
                json_data = json.loads(data_str)
                
                # Extract and display the CPU temperature (from the fTemp field)
                if "CpuInfo" in json_data and "fTemp" in json_data["CpuInfo"]:
                    temperatures = json_data["CpuInfo"]["fTemp"]
                    print(f"CPU Temperatures (Celsius): {temperatures}")
                    
                    # Optionally: Display the average temperature
                    average_temp = sum(temperatures) / len(temperatures)
                    print(f"Average CPU Temperature (Celsius): {average_temp:.2f}")
                    
                    # Display temperature of individual cores
                    for i, temp in enumerate(temperatures, 1):
                        print(f"Core {i} Temperature: {temp:.2f}°C")
                else:
                    print("Temperature data not found.")
                
                # Display other CPU Info
                if "CpuInfo" in json_data:
                    cpu_info = json_data["CpuInfo"]
                    print(f"CPU Name: {cpu_info.get('CPUName', 'Unknown')}")
                    print(f"CPU Speed: {cpu_info.get('fCPUSpeed', 'N/A')} MHz")
                    print(f"CPU Load: {cpu_info.get('uiLoad', 'N/A')}")
                    print(f"TDP: {cpu_info.get('uiTdp', 'N/A')} W")
                    print(f"Multiplier: {cpu_info.get('fMultiplier', 'N/A')}")
                    print(f"VID: {cpu_info.get('fVID', 'N/A')} V")
                    print(f"Fahrenheit Support: {'Yes' if cpu_info.get('ucFahrenheit') == 1 else 'No'}")
                    
                # Display Memory Info
                if "MemoryInfo" in json_data:
                    memory_info = json_data["MemoryInfo"]
                    print(f"Total Physical Memory: {memory_info.get('TotalPhys', 'N/A')} MB")
                    print(f"Free Physical Memory: {memory_info.get('FreePhys', 'N/A')} MB")
                    print(f"Memory Load: {memory_info.get('MemoryLoad', 'N/A')}%")
                
            except json.JSONDecodeError:
                print("Failed to decode JSON.")
        else:
            print("No data received from server.")
            
    except Exception as e:
        print(f"Error connecting to Core Temp server: {e}")

# Call the function
get_core_temp()
