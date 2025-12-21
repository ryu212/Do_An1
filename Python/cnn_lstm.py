import torch.nn as nn
class CNN_LSTM(nn.Module):
    def __init__(self, num_classes=5):
        super().__init__()
        self.net = nn.Sequential(
            nn.Conv1d(in_channels=1, out_channels = 64, kernel_size = 7, padding=3),
            nn.BatchNorm1d(64),
            nn.MaxPool1d(kernel_size=2),
            nn.Conv1d(in_channels=64, out_channels=128, kernel_size = 3, padding=1),
            nn.BatchNorm1d(128),
            nn.MaxPool1d(kernel_size=2),
            nn.Dropout(0.3),
            nn.Flatten(),
            nn.Linear(11520, 512), #update this when update windowsize
            nn.ReLU(),
            nn.Dropout(0.3)
            )
        self.lstm = nn.LSTM(input_size=512, hidden_size=128, num_layers=1, batch_first=True, bidirectional=True)
        self.fc = nn.Sequential(
           nn.AdaptiveAvgPool1d(output_size=256),
           nn.Flatten(),
           nn.Linear(256, 64),
           nn.ReLU(),
           nn.Linear(64, num_classes)
           )
    def forward(self, x):
        out = self.net(x)
        out = out.unsqueeze(1)
        out, _ = self.lstm(out)
        out = self.fc(out[:, -1, :])
        return out