import cv2
import numpy as np

W, H, FPS, N = 640, 480, 30, 1000
FONT = cv2.FONT_HERSHEY_SIMPLEX
FONT_SCALE = 5
TEXT_THICKNESS = 10
OUTLINE_KERNEL = np.ones((7, 7), np.uint8)

out = cv2.VideoWriter("counter.mp4", cv2.VideoWriter_fourcc(*"mp4v"), FPS, (W, H))
for i in range(N):
    frame = np.zeros((H, W, 3), np.uint8)
    frame[:] = (i % 224, (i * 3) % 224, (i * 7) % 224)
    text = str(i)
    (tw, th), base = cv2.getTextSize(text, FONT, FONT_SCALE, TEXT_THICKNESS)
    org = ((W - tw) // 2, (H + th) // 2)

    textMask = np.zeros((H, W), np.uint8)
    cv2.putText(textMask, text, org, FONT, FONT_SCALE, 255, TEXT_THICKNESS, cv2.LINE_AA)
    outlineMask = cv2.dilate(textMask, OUTLINE_KERNEL)
    frame[outlineMask > 0] = (0, 0, 0)
    cv2.putText(frame, text, org, FONT, FONT_SCALE, (255, 255, 255), TEXT_THICKNESS, cv2.LINE_AA)
    out.write(frame)
out.release()
