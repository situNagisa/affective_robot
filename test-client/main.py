import requests

# 定义 API 端点
url = 'http://134.175.177.58:9191/tem/upload-voice'

# 定义要上传的音频文件路径
file_path = '../xxx.wav'

# 使用 multipart/form-data 上传文件
with open(file_path, 'rb') as f:
    files = {'file': ('audio.wav', f, 'audio/wav')}
    response = requests.post(url, files=files)

# 打印响应内容
print(f'Status Code: {response.status_code}')
print(f'Response: {response.text}')
