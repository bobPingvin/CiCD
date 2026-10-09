#!/bin/bash
set -e

echo "=== 1. Сборка системы CI/CD и плагинов через CMake ==="
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cp ci-cd ..
cd ..

echo -e "\n=== 2. Проверка изолированных команд CLI ==="
# Сборка
./ci-cd build --source ./sample_app/src --output ./bin
# Запуск тестов
./ci-cd test --tests ./sample_app/tests
# Развертывание
./ci-cd deploy --source ./bin --target ./deploy

echo -e "\n=== 3. Проверка развернутого приложения ==="
./deploy/app
cat ./deploy/deploy_report.txt

echo -e "\n=== 4. Полный запуск пайплайна с плагинами (через JSON) ==="
./ci-cd run --config pipeline.json

echo -e "\n=== 5. Проверка остановки пайплайна при ошибке (Fail-fast) ==="
if ./ci-cd run --config pipeline_fail.json; then
    echo "ОШИБКА: Пайплайн не должен был завершиться успешно!"
    exit 1
else
    echo "УСПЕХ: Пайплайн корректно перехватил ошибку и остановил выполнение."
fi
