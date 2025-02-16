import sys
import os
from thrift import Thrift
from thrift.transport import TTransport
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol

sys.path.append(os.path.join(os.path.dirname(__file__), 'gen-py'))
from example import CalcService

def main():
    # 创建 Thrift 客户端
    transport = TSocket.TSocket("localhost", 9090)
    transport = TTransport.TBufferedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = CalcService.Client(protocol)

    try:
        # 打开传输
        transport.open()

        # 调用 add 方法
        response = client.add(3, 4)
        print(f"Result: {response}")

    except Thrift.TException as tx:
        print(f"ERROR: {tx.message}")

    finally:
        # 关闭传输
        transport.close()

if __name__ == "__main__":
    main()
