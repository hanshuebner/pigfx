;; -*- Lisp -*-

(defpackage :make-vt220-font
  (:use :cl :cl-gd))

(in-package :make-vt220-font)

(defun convert-font (&key (input-filename "vt220-raw.png") (output-filename "font10x20.bin"))
  (with-open-file (output output-filename
                          :direction :output
                          :if-does-not-exist :create
                          :if-exists :supersede
                          :element-type '(unsigned-byte 8))
    (with-image-from-file* (input-filename)
      (dotimes (i 256)
        (format t "~A~%" i)
        (let* ((i (case i
                    (0 32)
                    (32 0)
                    (otherwise i)))
               (src-x (+ 1 (* (floor i 16) 10)))
               (src-y (+ 2 (* (mod i 16) 12))))
          (dotimes (y 10)
            (labels ((pixel-at-p (cx cy)
                       (zerop (get-pixel (+ src-x cx) (+ src-y cy))))
                     (write-pixel (setp)
                       (format t "~A " (if setp "*" "."))
                       (write-byte (if setp 255 0) output))
                     (make-row ()
                       (format t "[")
                       (dotimes (x 8)
                         (write-pixel (or (pixel-at-p x y)
                                          (and (plusp x)
                                               (pixel-at-p (1- x) y)))))
                       (dotimes (x 2)
                         (write-pixel (pixel-at-p 7 y)))
                       (format t "]~%")))
              (make-row)
              (make-row))))))))
