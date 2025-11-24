from model import get_data, DNN
from torch.utils.data import DataLoader, TensorDataset
import torch
from esp_ppq.api import espdl_quantize_torch
from esp_ppq.executor.torch import TorchExecutor
from sklearn.preprocessing import StandardScaler, LabelEncoder
import torch
import numpy as np
import torch.nn as nn
from sklearn.model_selection import train_test_split


def collate_fn(batch):
    # TensorDataset 迭代的时候返回的是 Tuple(x, y), 量化的时候只需要x, 不需要label y。
    batch = batch[0].to(DEVICE)
    return batch

def seed_worker(worker_id):
    worker_seed = torch.initial_seed() % 2 ** 32
    np.random.seed(worker_seed)
    random.seed(worker_seed)

def init_weights(m):
    if isinstance(m, nn.Linear):
        torch.nn.init.xavier_uniform_(m.weight)
        m.bias.data.fill_(0.01)

if __name__ == "__main__":
    seed = 42
    ESPDL_MODEL_PATH = "model.espdl"
    INPUT_SHAPE = [1, 7]  # 1 个输入特征
    TARGET = "esp32s3"  # 量化目标类型，可选 'c', 'esp32s3' or 'esp32p4'
    NUM_OF_BITS = 8  # 量化位数
    DEVICE = "cpu"  # 'cuda' or 'cpu', if you use cuda, please make sure that cuda is available

    x, y = get_data()
    # dataloader shuffle必须设置为False。
    # 因为计算量化误差的时候会多次遍历数据集，如果shuffle是True的话，会得到错误的量化误差。
    scaler = StandardScaler()
    X = scaler.fit_transform(x)
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    print(X_train.shape, X_test.shape, y_train.shape, y_test.shape)
    # print(type(x_test), type(y_test)) # <class 'numpy.ndarray'> <class 'numpy.ndarray'>
    # print(x_test.shape, y_test.shape) # (200020, 7) (200020,)
    
    x_test = torch.tensor(X_test, dtype=torch.float32)
    print(x_test)
    y_test = torch.tensor(y_test, dtype=torch.float).view(-1, 1)
    print(y_test)
    dataset = TensorDataset(x_test, y_test)
    print("Dataset: \n")
    print(dataset[0])
    dataloader = DataLoader(dataset, batch_size=32, shuffle=False, worker_init_fn=seed_worker, generator=torch.Generator().manual_seed(seed))
    # print("DataLoader: ")
    # print(dataloader)

    model = DNN(7, 128, 1)
    model.load_state_dict(torch.load("human_vital_signs_model.pth"))
    model.eval()
    
    with torch.no_grad():
        for inputs, labels in dataloader:
            outputs = model(inputs)
            print(outputs)

    quant_ppq_graph = espdl_quantize_torch(
        model=model,
        espdl_export_file=ESPDL_MODEL_PATH,
        calib_dataloader=dataloader,
        calib_steps=32,  # 校准的步数
        input_shape=INPUT_SHAPE,  # 输入形状，批次为 1
        inputs=None,
        target=TARGET,  # 量化目标类型
        num_of_bits=NUM_OF_BITS,  # 量化位数
        collate_fn=collate_fn,
        dispatching_override=None,
        device=DEVICE,
        error_report=True,
        skip_export=False,
        export_test_values=True,
        verbose=1,  # 输出详细日志信息
    )

    criterion = nn.BCEWithLogitsLoss()
    # 原始模型测试集精度
    loss = 0
    for batch_x, batch_y in dataloader:
        y_pred = model(batch_x)
        loss += criterion(y_pred, batch_y)
    loss /= len(dataloader)
    print(f"origin model loss: {loss.item():.5f}")

    # 量化模型测试集精度
    executor = TorchExecutor(graph=quant_ppq_graph, device=DEVICE)
    loss = 0
    for batch_x, batch_y in dataloader:
        y_pred = executor(batch_x)
        loss += criterion(y_pred[0], batch_y)
    loss /= len(dataloader)
    print(f"quant model loss: {loss.item():.5f}")