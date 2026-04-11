#lang racket

(define x 42)
(define y 10)

(define (square n)
  (* n n))

(define (factorial n)
  (if (<= n 1)
      1
      (* n (factorial (- n 1)))))

(define (classify-number n)
  (cond
    [(< n 0) "negative"]
    [(= n 0) "zero"]
    [(< n 10) "small"]
    [else "large"]))

(define numbers (list 1 2 3 4 5))

(define squares
  (map (lambda (n) (square n)) numbers))

(let ([sum (+ x y)]
      [fact (factorial 5)])
  (display "Sum: ")
  (display sum)
  (newline)
  (display "Factorial: ")
  (display fact)
  (newline)
  (display "Squares: ")
  (display squares)
  (newline)
  (display "Classification of x: ")
  (display (classify-number x))
  (newline))

(if (> x y)
    (display "x is greater than y")
    (display "y is greater or equal to x"))