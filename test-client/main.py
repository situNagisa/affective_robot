import requests

# 发送 POST 请求
url = "http://119.91.44.126:9090/api/text/save"
data = {
    "id": 10086,
    "content": "123123"
}

response = requests.post(url, json="hello")  # 发送 JSON 数据

# 检查响应状态码
if response.status_code == 200:
    print("成功提交数据:")
    print(response.json())  # 假设返回的是 JSON 格式
else:
    print("请求失败:", response.status_code)