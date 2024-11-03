def hex_string_to_bytes(hex_string):
    """将十六进制字符串转换为字节对象"""
    hex_values = hex_string.split(',')
    byte_array = bytearray(int(h, 16) for h in hex_values)
    return bytes(byte_array)

def write_wav_file(output_filename, audio_data):
    """将字节数据写入 WAV 文件"""
    with open(output_filename, 'wb') as wav_file:
        wav_file.write(audio_data)  # 直接写入音频数据

def main(input_filename, output_filename):
    # 读取包含十六进制数据的文本文件
    with open(input_filename, 'r') as file:
        hex_content = file.read().strip()

    # 将十六进制字符串转换为字节数据
    audio_data = hex_string_to_bytes(hex_content)

    # 将字节数据写入 WAV 文件
    write_wav_file(output_filename, audio_data)
    print(f"WAV file '{output_filename}' has been created.")

if __name__ == "__main__":
    input_file = './xxx.txt'  # 替换为你的输入文本文件名
    output_file = 'xxx.wav'  # 输出 WAV 文件名
    main(input_file, output_file)