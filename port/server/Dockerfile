FROM python:3.13-alpine@sha256:7984cf56ceaddebe4fefeb75d64000b9f45d8b77b11a09b24524da6cdc3af0c8
ENV PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1
WORKDIR /app
COPY netlink_relay.py /app/netlink_relay.py
USER 65532:65532
EXPOSE 47800/udp
CMD ["python", "netlink_relay.py", "--port", "47800", "--max-rooms", "128"]
