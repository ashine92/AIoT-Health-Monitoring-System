import joblib

scaler = joblib.load("scaler.pkl")

print("Scaler type:", type(scaler))
print("Scaler mean:", scaler.mean_) # Trung bình (mean) của từng đặc trưng
print("Scaler scale:", scaler.scale_) # Độ lệch chuẩn (std)
print("Scaler var:", scaler.var_) # Phương sai
print("Scaler n_samples_seen:", scaler.n_samples_seen_) # Số mẫu huấn luyện ban đầu

