# ZMK 本地构建脚本（用 Docker 持久容器，west update 只跑一次）
$ErrorActionPreference = "Stop"
$IMAGE = "zmkfirmware/zmk-build-arm:stable"
$CONTAINER = "zmk-build"
$WORKSPACE = $PWD.Path

# 1. 创建并启动持久容器（仅首次）
$exists = docker ps -a --format "{{.Names}}" | Select-String -Quiet "^$CONTAINER$"
if (-not $exists) {
    Write-Host "创建构建容器..." -ForegroundColor Cyan
    docker run -d --name $CONTAINER -v "${WORKSPACE}:/workspace" -w /workspace $IMAGE sleep infinity | Out-Null
}
docker start $CONTAINER | Out-Null

# 2. 首次初始化 west 工作区（拉取 zmk/zephyr，较慢，只跑一次）
docker exec $CONTAINER test -d /workspace/.west
if ($LASTEXITCODE -ne 0) {
    Write-Host "初始化 west 工作区（首次较慢，拉取 zmk/zephyr）..." -ForegroundColor Cyan
    docker exec $CONTAINER bash -lc "west init -l config && west update"
}

# 3. 编译
Write-Host "开始编译..." -ForegroundColor Cyan
docker exec $CONTAINER bash -lc "west build -s zmk/app -b 'nice_nano//zmk' -d build -- -DSHIELD=numpad -DZMK_CONFIG=config -DZMK_EXTRA_MODULES=/workspace"
if ($LASTEXITCODE -eq 0) {
    Write-Host "完成。固件: build/zephyr/zmk.uf2" -ForegroundColor Green
}
