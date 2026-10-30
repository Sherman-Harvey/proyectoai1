
import os
import io
import csv
from datetime import datetime, timezone

from fastapi import FastAPI, Header, HTTPException
from fastapi.responses import StreamingResponse
from psycopg_pool import ConnectionPool

DATABASE_URL = os.getenv("DATABASE_URL")
API_KEY = os.getenv("ESP32_API_KEY")

pool = ConnectionPool(DATABASE_URL, min_size=1, max_size=3)
app = FastAPI()

@app.get("/health")
def health():
    return {"status": "ok"}

@app.post("/telemetry")
def telemetry(data: dict, x_api_key: str = Header(None)):
    if x_api_key != API_KEY:
        raise HTTPException(401, "Invalid API key")

    with pool.connection() as conn:
        with conn.cursor() as cur:
            cur.execute(
                """INSERT INTO telemetry
                (device_id, soil_moisture_pct, air_temperature_c,
                 air_humidity_pct, light_lux, pump_state, fan_state)
                VALUES (%s,%s,%s,%s,%s,%s,%s)""",
                (
                    data.get("device_id"),
                    data.get("soil_moisture_pct"),
                    data.get("air_temperature_c"),
                    data.get("air_humidity_pct"),
                    data.get("light_lux"),
                    data.get("pump_state"),
                    data.get("fan_state"),
                ),
            )
    return {"status": "stored"}

@app.get("/data.csv")
def data_csv(x_api_key: str = Header(None)):
    if x_api_key != API_KEY:
        raise HTTPException(401, "Invalid API key")

    with pool.connection() as conn:
        with conn.cursor() as cur:
            cur.execute(
                """SELECT created_at, device_id, soil_moisture_pct,
                          air_temperature_c, air_humidity_pct, light_lux,
                          pump_state, fan_state
                   FROM telemetry ORDER BY created_at ASC"""
            )
            rows = cur.fetchall()

    output = io.StringIO()
    writer = csv.writer(output)
    writer.writerow([
        "timestamp", "device_id", "soil_moisture_pct",
        "air_temperature_c", "air_humidity_pct", "light_lux",
        "pump_state", "fan_state"
    ])
    writer.writerows(rows)
    output.seek(0)

    return StreamingResponse(
        output,
        media_type="text/csv",
        headers={"Content-Disposition": "attachment; filename=telemetry.csv"}
    )