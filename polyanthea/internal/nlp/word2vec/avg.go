// Package word2vec - Word2vec reading and basic operations.
package word2vec

// Avg - Compute the average of some vectors
func Avg(vectors []*Vector) *Vector {
	if len(vectors) == 0 {
		return nil
	}
	dim := len(*vectors[0])
	avg := make(Vector, dim)
	n := float32(len(vectors))
	for d := range dim {
		var value float32
		for _, vec := range vectors {
			value += (*vec)[d]
		}
		avg[d] = value / n
	}
	return &avg
}
