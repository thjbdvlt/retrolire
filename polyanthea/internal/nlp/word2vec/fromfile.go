// Package word2vec - Word2vec reading and basic operations.
package word2vec

// Source code of this file is extracted from: https://github.com/sajari/word2vec
// Other source files in this repository are written by me (thjbdvlt).
// I removed functions that I don't use, change some names case.
//
// Original License:
//
// MIT License
// Copyright (c) 2024 kshard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

import (
	"bufio"
	"encoding/binary"
	"fmt"
	"io"
	"math"
)

// Vector represents a word vector.
type Vector []float32

// Model represents a word2vec Model and implements the Coser and Mapper interfaces.
type Model struct {
	Dim   int
	Words map[string]Vector
}

// fromReader creates a Model using the binary model data provided by the io.Reader.
func fromReader(r io.Reader) (*Model, error) {
	br := bufio.NewReader(r)
	var size, dim int
	n, err := fmt.Fscanln(r, &size, &dim)
	if err != nil {
		return nil, err
	}
	if n != 2 {
		return nil, fmt.Errorf("could not extract size/dim from binary model data")
	}
	m := &Model{
		Words: make(map[string]Vector, size),
		Dim:   dim,
	}
	raw := make([]float32, size*dim)
	for i := range size {
		w, err := br.ReadString(' ')
		if err != nil {
			return nil, err
		}
		w = w[:len(w)-1]
		v := Vector(raw[dim*i : m.Dim*(i+1)])
		if err := binary.Read(br, binary.LittleEndian, v); err != nil {
			return nil, err
		}
		v.normalise()
		m.Words[w] = v
		b, err := br.ReadByte()
		if err != nil {
			if i == size-1 && err == io.EOF {
				break
			}
			return nil, err
		}
		if b != byte('\n') {
			if err := br.UnreadByte(); err != nil {
				return nil, err
			}
		}
	}
	return m, nil
}

// normalises the vector in-place.
func (v Vector) normalise() {
	n := v.norm()
	for i := range v {
		v[i] /= n
	}
}

// computes the Euclidean norm of the vector.
func (v Vector) norm() float32 {
	var out float32
	for _, vx := range v {
		out += vx * vx
	}
	return float32(math.Sqrt(float64(out)))
}
