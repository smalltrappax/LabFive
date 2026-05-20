# syntax=docker/dockerfile:1
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ca-certificates python3 python3-venv \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN make clean && make app

RUN python3 -m venv /venv \
    && /venv/bin/pip install --no-cache-dir streamlit

ENV PATH="/venv/bin:${PATH}"

EXPOSE 8501
CMD ["streamlit", "run", "app.py", "--server.address=0.0.0.0", "--server.port=8501"]
