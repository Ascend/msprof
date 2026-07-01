# -------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This file is part of the MindStudio project.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#    http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
import argparse

def resnet50_train():
    import torch
    from src.model import ResNet50

    trainer = ResNet50(num_classes=10)
    fake_images = torch.randn(80, 3, 224, 224)
    fake_labels = torch.randint(0, 10, (80,))
    dataset = torch.utils.data.TensorDataset(fake_images, fake_labels)
    loader = torch.utils.data.DataLoader(dataset, batch_size=8, shuffle=True)
    trainer.train(loader, epochs=2, lr=1e-3, freeze_backbone=True)


def communication_train():
    from src.communication_train import train as communication_train_impl

    communication_train_impl()


# Map function names to actual functions
TRAIN_FUNCS = {
    "resnet50": resnet50_train,
    "communication": communication_train,
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train different models.")
    parser.add_argument(
        "--model", type=str, default="resnet50", choices=TRAIN_FUNCS.keys(), help="Model to train (default: resnet50)"
    )
    args = parser.parse_args()

    print(f"[INFO] Starting training for model: {args.model}")
    TRAIN_FUNCS[args.model]()
