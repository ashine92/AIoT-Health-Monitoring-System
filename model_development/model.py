import pandas as pd
import numpy as np
# import torch
import torch.nn as nn
import torch
from torch import optim
# from torch import optim
from torch.utils.data import TensorDataset, DataLoader
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler, LabelEncoder
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix, precision_score, recall_score, f1_score, roc_auc_score, roc_curve
import matplotlib.pyplot as plt
import seaborn as sns
import random
import joblib
import onnx
import onnxsim

# Define the DNN model
class DNN(nn.Module):
    def __init__(self, input_size, hidden_size, output_size):
        super(DNN, self).__init__()
        self.fc1 = nn.Linear(input_size, hidden_size)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(hidden_size, hidden_size)
        self.fc3 = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        out = self.fc1(x)
        out = self.relu(out)
        out = self.fc2(out)
        out = self.relu(out)
        out = self.fc3(out)
        return out
    
def get_data():
    # Load the dataset
    df = pd.read_csv('human_vital_signs_dataset_2024.csv')
    label_encoder = LabelEncoder()
    scaler = StandardScaler()
    # Preprocess the data
    df = df.drop_duplicates()
    df = df.drop(['Timestamp', 'Patient ID', 'Derived_HRV', 'Derived_Pulse_Pressure', 'Derived_BMI', 'Derived_MAP', 'Respiratory Rate', 'Systolic Blood Pressure', 'Diastolic Blood Pressure'], axis='columns')
    df['Gender'] = label_encoder.fit_transform(df['Gender'])
    df['Risk Category'] = label_encoder.fit_transform(df['Risk Category'])
    X = df.drop('Risk Category', axis='columns').values
    y = df['Risk Category'].values
    return X, y

# Define DataLoader with seed
def seed_worker(worker_id):
    worker_seed = torch.initial_seed() % 2**32
    np.random.seed(worker_seed)
    random.seed(worker_seed)

# Initialize model weights
def init_weights(m):
    if isinstance(m, nn.Linear):
        torch.nn.init.xavier_uniform_(m.weight)
        m.bias.data.fill_(0.01)

if __name__ == "__main__":
    label_encoder = LabelEncoder()
    scaler = StandardScaler()
    # Load and preprocess the data
    x, y = get_data()
    X = scaler.fit_transform(x)
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
    print(X_train.shape, X_test.shape, y_train.shape, y_test.shape)

    X_train_tensor = torch.tensor(X_train, dtype=torch.float32)
    y_train_tensor = torch.tensor(y_train, dtype=torch.float32).view(-1, 1)
    X_test_tensor = torch.tensor(X_test, dtype=torch.float32)
    y_test_tensor = torch.tensor(y_test, dtype=torch.float32).view(-1, 1)
    train_dataset = TensorDataset(X_train_tensor, y_train_tensor)
    test_dataset = TensorDataset(X_test_tensor, y_test_tensor)
    print(train_dataset)
    print(test_dataset)

    train_loader = DataLoader(
        train_dataset,
        batch_size=32,
        shuffle=True,
        worker_init_fn=seed_worker,
        generator=torch.Generator().manual_seed(42)
    )

    test_loader = DataLoader(
        test_dataset,
        batch_size=32,
        shuffle=False,
        worker_init_fn=seed_worker,
        generator=torch.Generator().manual_seed(42)
    )

    input_size = X_train.shape[1]
    print(input_size)  # This line was added to print the input size
    hidden_size = 128
    output_size = 1
    learning_rate = 0.0001
    num_epochs = 20

    model = DNN(input_size, hidden_size, output_size)

    # Initialize model weights
    model.apply(init_weights)
    # Define loss function and optimizer
    criterion = nn.BCEWithLogitsLoss()
    optimizer = optim.Adam(model.parameters(), lr=learning_rate)

    # Train the model
    train_losses = []
    test_losses = []

    for epoch in range(num_epochs):
        model.train()
        epoch_loss = 0
        for i, (inputs, labels) in enumerate(train_loader):
            outputs = model(inputs)
            loss = criterion(outputs, labels)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            epoch_loss += loss.item()
        train_losses.append(epoch_loss / len(train_loader))

        model.eval()
        test_loss = 0
        with torch.no_grad():
            for inputs, labels in test_loader:
                outputs = model(inputs)
                loss = criterion(outputs, labels)
                test_loss += loss.item()
        test_losses.append(test_loss / len(test_loader))

        print(f'Epoch [{epoch+1}/{num_epochs}], Train Loss: {train_losses[-1]:.4f}, Test Loss: {test_losses[-1]:.4f}')

    # Evaluate the model
    model.eval()
    y_true = []
    y_pred = []
    y_pred_proba = []

    with torch.no_grad():
        for inputs, labels in test_loader:
            outputs = model(inputs)
            predicted = torch.sigmoid(outputs) >= 0.5
            y_true.extend(labels.numpy())
            y_pred.extend(predicted.numpy())
            y_pred_proba.extend(torch.sigmoid(outputs).numpy())

    y_true = np.array(y_true).flatten()
    y_pred = np.array(y_pred).flatten()
    y_pred_proba = np.array(y_pred_proba).flatten()

    accuracy = accuracy_score(y_true, y_pred)
    precision = precision_score(y_true, y_pred)
    recall = recall_score(y_true, y_pred)
    f1 = f1_score(y_true, y_pred)
    roc_auc = roc_auc_score(y_true, y_pred_proba)
    conf_matrix = confusion_matrix(y_true, y_pred)

    print(f'Accuracy: {accuracy *100:.2f}%')
    print(f'Precision: {precision *100:.2f}%')
    print(f'Recall: {recall *100:.2f}%')
    print(f'F1 Score: {f1 *100:.2f}%')
    print(f'ROC AUC Score: {roc_auc *100:.2f}%')

    # Plot confusion matrix
    """
    plt.figure(figsize=(6, 4))
    sns.heatmap(conf_matrix, annot=True, fmt='d', cmap='Blues', xticklabels=['Low Risk', 'High Risk'], yticklabels=['Low Risk', 'High Risk'])
    plt.xlabel('Predicted')
    plt.ylabel('Actual')
    plt.title('Confusion Matrix')
    plt.show()

    # Plot ROC curve
    fpr, tpr, thresholds = roc_curve(y_true, y_pred_proba)
    plt.figure(figsize=(8, 6))
    plt.plot(fpr, tpr, label='ROC Curve')
    plt.plot([0, 1], [0, 1], linestyle='--', label='Random Guess')
    plt.xlabel('False Positive Rate')
    plt.ylabel('True Positive Rate')
    plt.title('ROC Curve')
    plt.legend()
    plt.show()
    """
    # Heart Rate, Body Temperature, Oxygen Saturation, Age, Gender, Weight (kg), Height (m)
    manual_data = [
        [96, 36.85263343, 97.12412468, 72, 0, 71.75897167, 1.603887873],
        [83, 36.044191, 98.584497, 84, 0, 79.295332, 1.672735]
    ]

    manual_data = np.array(manual_data)
    manual_data_scaled = scaler.transform(manual_data)
    manual_data_tensor = torch.tensor(manual_data_scaled, dtype=torch.float32)

    model.eval()
    with torch.no_grad():
        outputs = model(manual_data_tensor)
        predicted_probs = torch.sigmoid(outputs).numpy().flatten()
        predicted_classes = (predicted_probs >= 0.5).astype(int)

    predicted_risk_categories = label_encoder.inverse_transform(predicted_classes)
    results_df = pd.DataFrame({
        "Data Point": range(1, len(manual_data) + 1),
        "Predicted Risk Category": predicted_risk_categories,
    })

    print(results_df)

    model_save_path = 'human_vital_signs_model.pth'
    torch.save(model.state_dict(), model_save_path)

    dummy_input = torch.randn([1, 7], dtype=torch.float32)
    print(dummy_input)
    torch.onnx.export(
        model,
        dummy_input,
        'human_vital_signs_model.onnx',
        opset_version=12,
        input_names=['input'],
        output_names=['output'],
    )

    onnx_model = onnx.load_model('human_vital_signs_model.onnx')
    onnx.checker.check_model(onnx_model)
    onnx_model, check = onnxsim.simplify(onnx_model)
    assert check, "Simplified ONNX model could not be validated"
    onnx.save_model(onnx_model, 'human_vital_signs_model.onnx')


